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
#pragma ident "$Revision: 1.66 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* Solaris header files */
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/scsi/impl/uscsi.h>
#include <sys/scsi/generic/sense.h>
#include <sys/scsi/generic/commands.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>

/* SAM header files */
#include "pub/devstat.h"
#include "sam/types.h"
#include "aml/device.h"
#include "aml/proto.h"
#include "sam/devinfo.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/license.h"
#include "parser_utils.h"
#include "include/mgmt/hash.h"

#include "sam/sam_trace.h"

#define	DPNC(ch)			(isprint(ch) ? ch : ' ')
#define	VENDOR_LEN			9
#define	PRODUCT_LEN			17
#define	REVISION_LEN			5
#define	SCMD_MAX_INQUIRY_STD_LEN	0xff
#define	READ_ELEMENT_STATUS_DATALEN	0xffff
#define	SCMD_MAX_INQUIRY_PAGE80_LEN	0xff
#define	SCMD_MIN_INQUIRY_PAGE80_LEN	0x04
#define	SCMD_MAX_INQUIRY_PAGE83_LEN	0xff
#define	SCMD_MIN_INQUIRY_PAGE83_LEN	0x08
#define	PAGE83_CODE_SET_BINARY		1
#define	PAGE83_CODE_SET_ASCII		2

#define	IDENTIFIER_TYPE_T10		1
#define	IDENTIFIER_TYPE_EUI		2
#define	IDENTIFIER_TYPE_NAA		3

#define	DVCID				0x1
#define	DVCID_BYTE			6

#define	DEVFSADM_CMD		"/usr/sbin/devfsadm -i samst"
#define	MEDCHANGER_CFGADM_CMD "/usr/sbin/cfgadm -al | /usr/bin/grep med-changer"
#define	BOOTADM_CMD		"/usr/sbin/bootadm update-archive"


typedef struct devid {
	int type;
	char desc[128];
} devid_t;

/* SCSI command functions */
static int _scsi_inq_std(
	devdata_t *devdata,
	char *vendor, char *product, int *version);

static int _scsi_inq_pg83(devdata_t *devdata, sqm_lst_t **id_lst);

static int _parse_devid(
	char *desc, int desclen, int *id_type, char *id);

static int scsi_read_dev_element_status(
	devdata_t *devdata, char *vendor, char *product, sqm_lst_t **drive_lst);

/* parse network-attached parameter files */
// TBD - move these to a separate file
static int get_parameter_kv(char *path, sqm_lst_t **strlst);

static int parse_stk_param(sqm_lst_t *strlst,  stk_param_t **param);
static int parsekv_stk_devicepaths(char *str, stk_device_t **dev);
static int parsekv_stk_CAP(char *str, stk_cap_t *cap);
static int parsekv_stk_capacity(char *str, sqm_lst_t **capacity_lst);

static int parse_sony_param(sqm_lst_t *strlst,  nwlib_base_param_t **param);

static int parse_ibm3494_param(sqm_lst_t *strlst,  nwlib_base_param_t **param);

static int parse_adicgrau_param(sqm_lst_t *strlst,  nwlib_base_param_t **param);


/* generic library functions */
static int _open_device(char *devpath, boolean_t config, int *open_fd);
static int _reconfig(char *devpath);

static int process_fc();
/* LINTED E_STATIC_UNUSED */
static int process_fc_lib(library_t *lib, char *wwn_name);

static void free_drive_void(void *v) {
	free_drive((drive_t *)v);
}

static void free_library_void(void *v) {
	free_library((library_t *)v);
}

/*
 * Issue_cmd - uscsi command interface.
 * Given device data, i.e. logical device path, file descriptior and
 * devtype, issue a SCSI Request Block (SRB) to a robot or tape drive.
 */
int
issue_cmd(
devdata_t *devdata,	/*  device data such as path and openfd */
char *cdb,		/* command descriptor block (cdb). */
int cdblen,		/* number of cdb bytes.		   */
char *data,		/* datain or dataout buffer	   */
int *datalen,		/* buffer length		   */
boolean_t datain,	/* data xfer direction		   */
boolean_t required /* ARGSUSED */) /* display errors	   */
{
	struct uscsi_cmd cmd;
	struct scsi_extended_sense sense;
	int	st;

	if (ISNULL(devdata)) {
		return (-1);
	}
	Trace(TR_OPRMSG, "issue cmd entry. dev=%s", Str(devdata->logdevpath));
	memset(&cmd, 0, sizeof (struct uscsi_cmd));

	cmd.uscsi_cdb = cdb;
	cmd.uscsi_cdblen = cdblen;

	memset(data, 0, *datalen);
	cmd.uscsi_bufaddr = data;
	cmd.uscsi_buflen = *datalen;

	cmd.uscsi_rqbuf = (char *)&sense;
	cmd.uscsi_rqlen = sizeof (struct scsi_extended_sense);

	cmd.uscsi_flags = USCSI_RQENABLE;
	cmd.uscsi_flags |= (datain == B_TRUE ? USCSI_READ : USCSI_WRITE);
	if (devdata->verbose == B_FALSE) {
		cmd.uscsi_flags |= USCSI_SILENT;
	}
	Trace(TR_OPRMSG, "issuing scsi cmd");

	if ((st = ioctl(devdata->fd, USCSICMD, &cmd)) != 0) {

		samerrno = SE_IOCTL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), devdata->logdevpath);
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "issue scsi cmd failed, ioctl status[%d]: %s",
		    st, strerror(errno));
		return (-1);
	}

	*datalen = cmd.uscsi_buflen - cmd.uscsi_resid;

	Trace(TR_OPRMSG, "finished issuing scsi cmd");
	return (0);
}


/*
 * scsi standard inquiry (12h) - This command gets the SCSI version number,
 * name of the manufacturer and the product. This command will function even
 * if the device is unable to accept other types of scsi command requests.
 *
 * This command descriptor block (cdb) is 6 byte long, with the opcode set to
 * 12h, the transfer length is set to ff and all other bytes are set to zero.
 * This represents a request for standard INQUIRY where 255 bytes or less are
 * expected.
 * e.g. cdb[] = {0x12, 0, 0, 0, ff, 0}
 *
 * This function will return a FAILURE (-1) if the scsi response is invalid,
 * less than 32 characters in length, or if the vendor or product id are empty.
 */
static int
_scsi_inq_std(
devdata_t *devdata,	/* INPUT - device data such as path and openfd */
char *vendor,		/* OUTPUT - VendorID[VENDOR_LEN] */
char *product,		/* OUTPUT - ProductID[PRODUCT_LEN] */
int *version)		/* OUTPUT - SCSI Version */
{
	int	datalen = SCMD_MAX_INQUIRY_STD_LEN;
	char	data[SCMD_MAX_INQUIRY_STD_LEN];
	char	cdb[] = {
			0x12, 0, 0, 0, (char)SCMD_MAX_INQUIRY_STD_LEN, 0};

	char	buf[32] = {0};

	Trace(TR_OPRMSG, "issuing a standard scsi inquiry");

	if (ISNULL(devdata, vendor, product, version)) {

		Trace(TR_ERR, "scsi inq std failed: %s", samerrmsg);
		return (-1);
	}

	if ((issue_cmd(devdata, cdb, 6, data, &datalen,
	    DATAIN, REQUIRED)) != 0) {
		Trace(TR_ERR, "failed to issue std inquiry cmd");
		return (-1);
	}

	/* need at least 32 bytes */
	if (datalen < 32) {
		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "std_inquiry short read %d", datalen);
		return (-1);
	}
	devdata->devtype = data [0] & 0x1f;
	*version = data[2] & 0xff;

	/* Manufacturer information is 8 bytes, from 8-15 */
	strlcpy(buf, data+8, 9);

	/* remove leading and trailing spaces from vendor id */
	if (str_trim(buf, vendor) == NULL) {
		samerrno = SE_STR_TRIM_CALL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry standard failed: %s", samerrmsg);
		return (-1);
	}

	/* Product is 16 bytes, from 16-31 */
	strlcpy(buf, data+16, 17);

	/* remove leading and trailing spaces from product id */
	if (str_trim(buf, product) == NULL) {
		samerrno = SE_STR_TRIM_CALL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry standard failed: %s", samerrmsg);
		return (-1);
	}

	if ((strlen(product) == 0) || (strlen(vendor) == 0)) {
		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "empty scsi inquiry response: %s", samerrmsg);
		return (-1);

	}

	Trace(TR_OPRMSG, "scsi inq std successful."
	    "version=%02x, vendor=%s, product=%s", version, vendor, product);

	return (0);
}


/*
 * scsi inquiry command (12h) page code 80 - This command gets the unit serial
 * number.
 *
 * This command descriptor block (cdb) is 6 byte long, with the opcode set to
 * 12h, vital product data field (EVPD) set to 1, the page code set to 80h, and
 * the transfer length set to ff
 * e.g. cdb[] = {0x12, 1, 80, 0, ff, 0}
 *
 * This function will return a FAILURE (-1) if the scsi response is invalid,
 * less than 32 characters in length, or if the serial number is empty.
 */
int
scsi_inq_pg80(
devdata_t *devdata,	/* INPUT - device data such as path and openfd */
char *serial_no)	/* OUTPUT- serial number[SCMD_MAX_INQUIRY_PAGE80_LEN] */
{

	char	cdb [] = {
		0x12, 1, (char)0x80, 0, (char)SCMD_MAX_INQUIRY_PAGE80_LEN, 0};
	char	data[SCMD_MAX_INQUIRY_PAGE80_LEN];
	int	datalen = SCMD_MAX_INQUIRY_PAGE80_LEN;
	int	len = 0;
	char 	s[SCMD_MAX_INQUIRY_PAGE80_LEN];

	Trace(TR_OPRMSG, "inquiring unit serial number");

	if (ISNULL(devdata, serial_no)) {
		Trace(TR_ERR, "scsi inquiry pg 80 failed: %s", samerrmsg);
		return (-1);
	}
	if (issue_cmd(
	    devdata, cdb, 6, data, &datalen, DATAIN, REQUIRED) != 0) {

		Trace(TR_ERR, "scsi inquiry pg 80 failed: %s", samerrmsg);
		return (-1);
	}
	if (datalen < SCMD_MIN_INQUIRY_PAGE80_LEN) {

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry pg 80 failed: "
		    "serial number could not be obtained");
		return (-1);
	}
	/*
	 * (datalen - 4) is the size of the buffer space available
	 * for the product serial number.  So data[3] (ie. product
	 * serial number) should be <= (datalen - 4).
	 */
	if (data[3] > (datalen - 4)) {

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inq pg80 failed: %s", samerrmsg);
		return (-1);
	}
	len = data[3];

	Trace(TR_MISC, "scsi_inq_pg80 datalen=%d, length=%d", datalen, len);

	strlcpy(s, data + 4, len + 1);

	/* trim leading and trailing spaces */
	if (str_trim(s, serial_no) == NULL) {

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inq pg80 failed: %s", samerrmsg);
		return (-1);
	}

	if (strlen(serial_no) == 0) {

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inq pg80 failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "scsi pg 80 inquiry serial no. = %s", serial_no);
	return (0);
}

/*
 * Read element status (B8h) - This function issues the READ ELEMENT STATUS
 * command to get a detailed overview of the data transfer elements (DTE) in
 * the media changer. A DTE represents the interface between the media changer
 * and a data transfer device (e.g., a removable media optical disk drive or
 * tape drive). The READ ELEMENT STATUS command response for each data transfer
 * element may provide the identity of the data transfer device serviced by the
 * media changer device. This support is optional since a data transfer device
 * is not required to be a SCSI device.
 *
 * Support for the READ ELEMENT STATUS command is mandatory for Media Changers.
 *
 * The READ ELEMENT STATUS command is 12 byte long, opcode = B8h, function unit
 * type = 4h, to indicate that the DTE are to be listed. In order to get all the
 * data transfer elements, set the first element address = 0h, number of
 * elements = FFFFh, and data length = FFFFFFh
 * i.e. cdb[] = B8, 4, 0, 0, FF, FF, 0, 0, FF, FFFF, 0, 0
 *
 * A device ID (DVCID) bit (byte 6, bit 0) of one specifies that the device
 * server shall return device identifiers, if available. A DVCID bit of zero
 * specifies that the target shall not return device identifiers. If the DVCID
 * is set to one and the device ID feature is not supported by the media changer
 * then CHECK CONDITION status shall be returned. The sense key shall be set to
 * ILLEGAL REQUEST and the additional sense code set to INVALID FIELD IN CDB.
 */
static int
scsi_read_dev_element_status(
devdata_t *devdata,	/* INPUT - device data such as path and openfd */
char *vendor,
char *product,
sqm_lst_t **drive_serialno_lst) /* OUTPUT - list of drive serial numbers  */
{
	char cdb [] = {
		(char)0xb8,
		4,
		0, 0,
		(char)0xff, (char)0xff,
		0,
		0, (char)0xff, (char)0xffff,
		0,
		0};

	char data[READ_ELEMENT_STATUS_DATALEN] = {0};
	int datalen = READ_ELEMENT_STATUS_DATALEN;
	int num_elements, step;
	int i, j;

	int id_type;
	char id[BUFSIZ];	/* check if this can be smaller */
	char *cstr;
	boolean_t l100 = B_FALSE;

	if (ISNULL(devdata, drive_serialno_lst)) {

		Trace(TR_ERR, "scsi read element status failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "send read element status command");

	/*
	 * for L100 libraries, the DVCID is not in the standard location, so
	 * special handling is required for parsing the command output.
	 */
	if ((strncmp(vendor, "ATL", 3) == 0) &&
	    strncmp(product, "M2500", 5) == 0) {
		cdb[1] |= 0x10;
		cdb[11] |= 0x80;
		l100 = B_TRUE;
	}

	/* A SCSI-3 aware robot can get DVCID for a data transport */
	cdb[DVCID_BYTE] |= DVCID;

	if (issue_cmd(
	    devdata, cdb, 12, data, &datalen, DATAIN, OPTIONAL) != 0) {

		/*
		 * TBD: SAM-FS supports the Plasmon G-Enterprise UDO/MO library
		 * if it is configured to the G-Enterprise mode, element address
		 * scheme 1, and barcode type 2 or 3
		 */

		/* try again: set DVCID = 0, fallback for old scsi-2 robots */
		cdb[DVCID_BYTE] &= ~DVCID;
		if (issue_cmd(
		    devdata, cdb, 12, data, &datalen, DATAIN, REQUIRED) != 0) {

			Trace(TR_ERR, "scsi read element status failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	*drive_serialno_lst = lst_create();
	if (*drive_serialno_lst == NULL) {
		Trace(TR_ERR, "scsi read element status failed: %s", samerrmsg);
		return (-1);

	}

	num_elements = (data[2] << 8) | data[3];
	/* last storage address of the medium in this list */
	step = (data[10] << 8) | data[11];

	Trace(TR_OPRMSG, "Number of elements in the list: %d,"
	    "last storage address: %x", num_elements, step);

	/* TBD: Find documentation for limits on length */
	if (datalen < ((num_elements - 1) * step) + 12 + 16) {

		lst_free(*drive_serialno_lst);
		Trace(TR_ERR, "read element status failed: %s", samerrmsg);
		return (-1);
	}
	/*
	 * The element status data consists of an eight byte header which is
	 * followed by the element pages. The element pages themselves contain
	 * an eight byte header and one or more element descriptors.
	 */
	for (i = 0, j = 16; /* element status header + element page header */
	    i < num_elements; i++, j += step) {

		boolean_t found = B_FALSE;

		if (l100 == B_TRUE) {
			/* bytes 52-61 contain the Tape Drive Serial Number */
			if (data[j + 52] != '\0') {
				strlcpy(id, &data[j + 52], BUFSIZ);
			} /* serial number not found */

			/* SMC-2 specification indicates one id descriptor */
		} else if (cdb[DVCID_BYTE] & DVCID) {

			if (_parse_devid(
			    &data[j+12],
			    (datalen - j+12),
			    &id_type,
			    id) != 0) {

				lst_free_deep(*drive_serialno_lst);
				Trace(TR_ERR, "read element status failed: %s",
				    samerrmsg);
				return (-1);
			}
			found = B_TRUE;
		}
		/*
		 * In the future if resources are available a new branch
		 * could be added to provide support for Plasmon. This branch
		 * would be used when vendor equals "Plasmon". The media type
		 * can be found at &data[j+16].
		 * Format 1: No bar codes requested:
		 * volTag = 0, BarCodes = 0
		 */
		if (!found) {
			Trace(TR_ERR, "Drive Id could not be obtained");
			lst_free_deep(*drive_serialno_lst);
			Trace(TR_ERR, "read element status failed: %s",
			    samerrmsg);
			return (-1);
		}

		cstr = copystr(id);
		if (cstr == NULL) {
			lst_free_deep(*drive_serialno_lst);
			Trace(TR_OPRMSG, "read element status failed: %s",
			    samerrmsg);
			return (-1);
		}

		if (lst_append(*drive_serialno_lst, cstr) != 0) {
			free(cstr);
			lst_free_deep(*drive_serialno_lst);
			Trace(TR_OPRMSG, "read element status failed: %s",
			    samerrmsg);
			return (-1);
		}
	}
	Trace(TR_MISC, "finished read element status");
	return (0);
}


/*
 * TBD: Find the spec for 'read dev element status' to check if the same
 * parse pg83 (product vital data) code can be used to parse the media changer
 * device element id as well... (the 4.6 code used this for both)
 *
 * Depending on the IDENTIFIER TYPE, i.e. desc[1] & 0xf
 * 0h - Vendor specific
 * 1h - T10 vendor ID based
 * 2h - EUI-64 based
 * 3h - NAA
 * 4h - Relative target port identifier
 * 5h - Target port group
 * 6h - Logical unit group
 * 7h - MD5 logical unit identifier
 * 8h - SCSI name string
 *
 * the descriptor will be parsed differently. The parse_id functionality is
 * incomplete, in that it only checks if id types are 1, 2, or 3.
 *
 *
 * the current implementation is based on the existing 4.6/4.5 code. It has
 * been tested with only a limited set of devices
 *
 * This function will return a FAILURE (-1) if the scsi response is invalid,
 * if desc less than 3 characters in length, or if the id is empty.
 */
static int
_parse_devid(
char *desc,	/* INPUT - identification descriptor */
int desclen,	/* INPUT - descriptor length */
int *id_type,	/* OUTPUT - identifier type */
char *id)	/* OUTPUT - serial number or wwn - max len 1024 */
{
	int code_set;
	char s[BUFSIZ] = {0};
	int length = 0, i;
	char *ptr;

	Trace(TR_OPRMSG, "parsing identifier descriptor");

	if (ISNULL(desc, id_type, id)) {
		Trace(TR_ERR, "parse devid failed: %s", samerrmsg);
		return (-1);
	}
	if (desclen < 3) {
		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "parse devid failed: %s", samerrmsg);
		return (-1);
	}

	code_set = desc[0] & 0xf;
	*id_type = desc[1] & 0xf;

	Trace(TR_OPRMSG, "code-set: %x, association: %x, id: %x",
	    code_set, (desc[2] >> 4) & 0x3, *id_type);

	if (desclen >= 4) {
		length = desc[3];
	} else {
		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "parse devid failed: %s", samerrmsg);
		return (-1);
	}

	for (i = 0; i < length; i++) {
		if (code_set == PAGE83_CODE_SET_BINARY) {

			/* save as unsigned hex */
			snprintf(s, BUFSIZ, "%s%02x",
			    s, (unsigned char)desc[i+4]);

		} else if (code_set == PAGE83_CODE_SET_ASCII) {

			s[i] = DPNC(desc[i+4]);
		} /* INCOMPLETE: what about other code-sets?? */
	}
	s[i] = '\0';

	ptr = s; /* temp pointer to id */
	switch (*id_type) {
		case IDENTIFIER_TYPE_T10:
		case IDENTIFIER_TYPE_EUI:
			if (length > 24) {

				/* extract from the 24 char for the id */
				ptr += 24;
				/* strlcpy(t, &s[24], BUFSIZ); */
			}
			break;

		case IDENTIFIER_TYPE_NAA:
		default:
			break;
	}

	if (str_trim(ptr, id) == NULL) {
		samerrno = SE_STR_TRIM_CALL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "parse devid failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "parse id complete: %s", id);
	return (0);
}


/*
 * scsi inquiry (12h) page code 83 - This command gets the vital product data
 * (VPD) page structure and the VPD pages that are applicable to all SCSI
 * devices.
 * This command descriptor block (cdb) is 6 byte long, with the opcode set to
 * 12h, vital product data field (EVPD) set to 1, the page code set to 83h, and
 * the transfer length set to ff
 * e.g. cdb[] = {0x12, 1, 80, 0, ff, 0}
 *
 * It is possible to have multiple identifiers to the same device, these are
 * returned in a list
 *
 */
static int
_scsi_inq_pg83(
devdata_t *devdata,	/* INPUT - device data such as path and openfd */
sqm_lst_t **id_lst)	/* OUTPUT - list of identifiers (devid_t) */
{

	char cdb[] = {0x12, 1, (char)0x83, 0,
		(char)SCMD_MAX_INQUIRY_PAGE83_LEN, 0};

	char data[SCMD_MAX_INQUIRY_PAGE83_LEN];
	int datalen = SCMD_MAX_INQUIRY_PAGE83_LEN;
	int usedlen = 0;
	int dlen = 0;
	char *dblk;
	devid_t *id = NULL;

	Trace(TR_OPRMSG, "issuing device identification inquiry.");

	if (ISNULL(devdata, id_lst)) {
		Trace(TR_ERR, "scsi inquiry page 83 failed: %s", samerrmsg);
		return (-1);
	}
	if (issue_cmd(devdata, cdb, 6, data, &datalen, DATAIN, REQUIRED) != 0) {
		Trace(TR_ERR, "scsi inquiry page 83 failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Validate the page 0x83 data
	 * Standards Spec. (spc3r23.pdf) T10/1416-D SCSI Primary Commands
	 * 7.6.3.1 Device Identification VPD Page (Page 323)
	 * Also see on10 source - usr/src/common/devid/devid_scsi.c
	 *
	 * Peripheral device type (bits 0 - 4) should not be 0x1Fh (Unknown)
	 * page length field should contain a non zero length value
	 * and not be greater than 255 bytes
	 */
	if ((data[0] & 0x1F) == 0x1F) {

		samerrno = SE_UNKNOWN_DEVICE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry page 83 failed: %s", samerrmsg);
		return (-1);
	}

	if ((data[2] == 0) && (data[3] == 0)) {
		/* length field is 0 */

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry page 83 failed: length field is 0");
		return (-1);
	}

	if (data[3] > (SCMD_MAX_INQUIRY_PAGE83_LEN - 3)) {
		/* length field exceeds expected size of 255 bytes */

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry page 83 failed:"
		    "length field exceeds expected size of 255 bytes");
		return (-1);
	}

	if (datalen < SCMD_MIN_INQUIRY_PAGE83_LEN) {

		samerrno = SE_INVALID_SCSI_RESPONSE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "scsi inquiry page 83 failed:"
		    "length is too short");
		return (-1);
	}

	*id_lst = lst_create();
	if (*id_lst == NULL) {
		Trace(TR_ERR, "scsi inquiry page 83 failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * convert device identification data to displayable format
	 *
	 * it is possible to have multiple descriptors blocks. Length of
	 * each descriptor block is contained in the descriptor header.
	 */
	dblk = &data[4];
	while (usedlen < data[3]) {

		dlen = dblk[3];
		if (dlen <= 0) {

			lst_free_deep(*id_lst);
			samerrno = SE_INVALID_SCSI_RESPONSE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "scsi inquiry page 83 failed:"
			    "length of identifier descriptor is invalid");
			return (-1);
		}
		if ((usedlen + dlen) > data[3]) {

			lst_free_deep(*id_lst);
			samerrno = SE_INVALID_SCSI_RESPONSE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "scsi inquiry page 83 failed:"
			    "length is greater than expected");
			return (-1);
		}

		id = mallocer(sizeof (devid_t));
		if (id == NULL) {

			lst_free_deep(*id_lst);
			Trace(TR_ERR, "scsi inquiry page 83 failed: %s",
			    samerrmsg);
			return (-1);
		}
		memset(id, 0, sizeof (devid_t));

		if (_parse_devid(
		    dblk,
		    dlen,
		    &(id->type),
		    id->desc) != 0) {

			free(id);
			lst_free_deep(*id_lst);
			Trace(TR_ERR, "scsi inquiry page 83 failed:"
			    "unable to parse identifier descriptor");
			return (-1);
		}

		if (lst_append(*id_lst, id) != 0) {

			free(id);
			lst_free_deep(*id_lst);
			Trace(TR_ERR, "scsi inquiry page 83 failed: %s",
			    samerrmsg);
			return (-1);
		}

		/*
		 * advance to the next descriptor block,
		 * the descriptor block size is <desc header> + <desc data>
		 * <desc header> is equal to 4 bytes
		 * <desc data> is available in desc[3]
		 */
		dblk = &dblk[4 + dlen];
		usedlen += (dlen + 4);

	}
	Trace(TR_OPRMSG, "scsi inquiry page 83 completed");
	return (0);
}


/*
 * get supported media
 * Reads the inquiry.conf to get a list of media types supported by SAM
 *
 * returns a list of medias_type_t (vendor_id, product_id, sam_id)
 * No RPC support for this function
 */
int
get_supported_media(
sqm_lst_t **media_list) /* OUTPUT - a list of medias_type_t */
{
	FILE *file_ptr;
	medias_type_t *media;
	char *inq_delims = ",";
	char *str, *p;
	char out_str[512];
	char *pp = out_str;
	char temp_buf[BUFSIZ];
	char type_detail[3][50];
	int i, ii;
	char buf[BUFSIZ];
	char *last;
	char *ptr = NULL;

	Trace(TR_OPRMSG, "getting a list of SAM supported media type");

	if (ISNULL(media_list)) {
		Trace(TR_ERR, "get supported media failed: %s", samerrmsg);
		return (-1);
	}

	if ((file_ptr = fopen(INQUIRY_PATH, "r")) == NULL) {
		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), INQUIRY_PATH);
		Trace(TR_ERR, "get supported media failed: %s", samerrmsg);
		return (-1);
	}

	*media_list = lst_create();
	if (*media_list == NULL) {
		Trace(TR_ERR, "get supported media failed: %s", samerrmsg);
		fclose(file_ptr);
		return (-1);
	}

	while (fgets(buf, BUFSIZ, file_ptr) != NULL) {
		str = &buf[0];
		if ((str[0] == '#') || (str[0] == '\n')) {
			continue;
		}
		p = strtok_r(str, inq_delims, &last);
		ii = 0;
		while (p != NULL) {
			pp = str_trim(p, out_str);
			if (pp[0] != '"') {
				break;
			}
			for (i = 1; i < strlen(pp); i++) {
				temp_buf[i-1] = pp[i];
				if (pp[i] == '"') {
					temp_buf[i-1] = '\0';
					break;
				}
			}
			strlcpy(type_detail[ii], temp_buf,
			    sizeof (type_detail[ii]));
			p = strtok_r(NULL, inq_delims, &last);
			ii++;
		}

		if (strlen(type_detail[0]) > 0) {
			media = (medias_type_t *)
			    mallocer(sizeof (medias_type_t));
			if (media == NULL) {

				Trace(TR_ERR, "Get supported media failed:%s",
				    samerrmsg);
				lst_free_deep_typed(*media_list,
				    FREEFUNCCAST(free_medias_type));
				fclose(file_ptr);
				return (-1);
			}
			memset(media, 0, sizeof (medias_type_t));

			ptr = copystr(type_detail[0]);
			if (ptr == NULL) {

				Trace(TR_ERR, "Get supported media failed:%s",
				    samerrmsg);
				free_medias_type(media);
				lst_free_deep_typed(*media_list,
				    FREEFUNCCAST(free_medias_type));
				fclose(file_ptr);
				return (-1);
			}
			media->vendor_id = ptr;
			ptr = copystr(type_detail[1]);
			if (ptr == NULL) {

				Trace(TR_ERR, "Get supported media failed:%s",
				    samerrmsg);
				free_medias_type(media);
				lst_free_deep_typed(*media_list,
				    FREEFUNCCAST(free_medias_type));
				fclose(file_ptr);
				return (-1);
			}
			media->product_id = ptr;
			ptr = copystr(type_detail[2]);
			if (ptr == NULL) {

				Trace(TR_ERR, "Get supported media failed:%s",
				    samerrmsg);
				free_medias_type(media);
				lst_free_deep_typed(*media_list,
				    FREEFUNCCAST(free_medias_type));
				fclose(file_ptr);
				return (-1);
			}
			media->sam_id = ptr;

			if (lst_append(*media_list, media) != 0) {

				Trace(TR_ERR, "Get supported media failed:%s",
				    samerrmsg);
				free_medias_type(media);
				lst_free_deep_typed(*media_list,
				    FREEFUNCCAST(free_medias_type));
				fclose(file_ptr);
				return (-1);
			}
		}
	}

	fclose(file_ptr);

	Trace(TR_OPRMSG, "finished getting a list of SAM supported media type");
	return (0);
}


/*
 * get_equ_type() given the vendor id and product id
 * If no match is found, equ_type is set to UNDEFINED_MEDIA_TYPE.
 *
 * The supported media list serves both as an input and output. If supported
 * media is NULL, then this function gets the list of supported media from
 * inquiry.conf and saves it in supported_media
 *
 * If supported_media is not NULL, it is used as is, and no modifications
 * are made to it. This saves time and improves performance by not having
 * to reprocess the inquiry.conf repeatedly when this function is invoked
 * in a loop.
 *
 * Note: From inquiry.conf(4) manpage:
 * During device identification, the vendor_id and product_id values are only
 * compared through the length of the string supplied in the inquiry.conf file
 * TBD: To insure a correct match, the supported_media list should be ordered
 * with longer names first.
 */
int
get_equ_type(
char *vendor_id,
char *product_id,
sqm_lst_t **supported_media,	/* IN + OUT - should be freed by caller */
char *equ_type)			/* OUTPUT -  use uname_t - 32 chars */
				/* a 2 char mnemonic or 'unknown type' */
{

	node_t *media_node = NULL;
	medias_type_t *media = NULL;
	int len_p, len_v;

	if (ISNULL(supported_media, vendor_id, product_id, equ_type)) {
		return (-1);
	}

	if (ISNULL(*supported_media)) {
		/*
		 * Read inquiry.conf to get the list of device types that are
		 * supported by SAM, the supported media type are saved and
		 * returned to the caller to avoid reading the inquiry.conf
		 * repeatedly.
		 */
		if (get_supported_media(supported_media) != 0) {
			return (-1);
		}
	}

	/*
	 * map the input vendor and product id with the vendor id & product id
	 * of the supported media types to get the equipment type field, a
	 * 2-character code to identify a particular removable media device
	 */
	media_node = (*supported_media)->head;
	while (media_node != NULL) {
		media = (medias_type_t *)media_node->data;

		len_p = strlen(media->product_id);
		len_v = strlen(media->vendor_id);

		if ((strncmp(vendor_id, media->vendor_id, len_v) == 0) &&
		    (strncmp(product_id, media->product_id, len_p) == 0)) {

			strlcpy(equ_type,
			    dt_to_nm(samid_to_dt(media->sam_id)),
			    sizeof (uname_t));
			return (0);

		}
		media_node = media_node->next;
	}
	strcpy(equ_type, UNDEFINED_MEDIA_TYPE);
	return (0);
}

/*
 * reconfigure the storage using "cfgadm -c configure %s", the library device
 * path is provided as input
 */
static int
_reconfig(char *devpath) {

	pid_t pid;
	uint64_t c, t, u; /* cXtXuX - handle hex device entries */
	int status;
	char cmd[PATH_MAX] = {0};

	if (ISNULL(devpath)) {
		Trace(TR_ERR, "reconfiguration failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "issuing cmd to reconfig device path: %s", devpath);
	/*
	 * The devpath is given as /dev/samst/cXtXuX. To configure, the
	 * controller information is required. the cmd for configuring the
	 * device is "cfgadm -c configure cX"
	 */

	if (sscanf(devpath, SAMSTDIR"/c%xt%xu%x", &c, &t, &u) != 3) {
		/* invalid device path */
		samerrno = SE_INVALID_DEVICE_PATH;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), devpath);
		Trace(TR_ERR, "reconfiguration failed: %s", samerrmsg);
		return (-1);
	}

	snprintf(cmd, sizeof (cmd), "/usr/sbin/cfgadm -c configure c%d", c);

	Trace(TR_OPRMSG, "Issuing cmd: %s", cmd);

	pid = exec_get_output(cmd, NULL, NULL);

	if (pid < 0) {
		samerrno = SE_EXEC_CFGADM_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "reconfiguration failed: %s", samerrmsg);
		return (-1);
	}
	if ((pid = waitpid(pid, &status, 0)) < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), cmd);
		Trace(TR_PROC, "reconfiguration failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "reconfiguration complete");
	return (0);
}

/* open a device */
static int
_open_device(
char *devpath,		/* INPUT - device path */
boolean_t config,	/* INPUT - reconfigure using cfgadm ? */
int *open_fd)		/* OUTPUT - open file descriptor */
{

	int	fd;

	if (ISNULL(devpath, open_fd)) {
		return (-1);
	}

	Trace(TR_OPRMSG, "open device path[%s]", devpath);
	if (strlen(devpath) > PATH_MAX) {

		samerrno = SE_INVALID_DEVICE_PATH;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), devpath);
		Trace(TR_ERR, "Invalid logical device path length");
		return (-1);
	}

	if ((fd = open(devpath, O_RDONLY| O_NDELAY)) == -1) {
		/*
		 * If device is busy, return error. If devpath begins with
		 * /dev/samst, indicating that the device is a library,
		 * reconfigure using cfgadm
		 */
		if (errno == EBUSY)  {
			samerrno = SE_DEVICE_BUSY;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), devpath);
			Trace(TR_ERR, "opn device failed: %s", samerrmsg);
			return (-1);
		}
		if (!config) {
			samerrno = SE_DEVICE_NOT_AVAILABLE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), devpath);
			Trace(TR_ERR, "opn device failed: %s", samerrmsg);
			return (-1);
		}
		/* re-configure device and retry */
		_reconfig(devpath);
		if ((fd = open(devpath, O_RDONLY| O_NDELAY)) == -1) {
			Trace(TR_OPRMSG, "%s not available even after reconfig",
			    devpath);
			samerrno = SE_DEVICE_NOT_AVAILABLE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), devpath);
			return (-1);
		}
	}
	*open_fd = fd;
	return (0);
}


/*
 * discovery a library by path
 */
int
discover_library_by_path(
ctx_t *ctx,		/* ARGSUSED */
library_t **lib,	/* OUTPUT - library */
upath_t libpath)	/* INPUT - library path to discover */
{
	struct	stat64  statbuf;
	node_t	*node_p, *tnode;
	sqm_lst_t	*supported_media	= NULL;
	sqm_lst_t	*drive_serialno_lst	= NULL;
	int status;
	devdata_t devdata;
	char *pp;
	char linkbuf[PATH_MAX+1]	= {0};

	if (ISNULL(lib)) {
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	if (strlen(libpath) == 0) {
		Trace(TR_ERR, "discover library by path failed: %s",
		    "library path is empty");
		return (-1);
	}

	memset(&devdata, 0, sizeof (devdata_t));

	Trace(TR_MISC, "discovering library by path %s", libpath);

	status = lstat64(libpath, &statbuf);
	if ((status == 0) && (S_ISLNK(statbuf.st_mode))) {
		status = readlink(libpath, linkbuf, sizeof (linkbuf));
		linkbuf[status] = '\0';	/* readlink may not terminate */
	} else {
		strlcpy(linkbuf, libpath, sizeof (linkbuf));
	}

	if (status < 0) {
		samerrno = SE_DEVICE_NOT_AVAILABLE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), libpath);

		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	/* scsi path - something like /devices/pci@1f,4000/scsi@4/ */
	pp = strstr(linkbuf, "samst");
	if (pp != NULL) {
		*pp = '\0';
	}

	*lib = (library_t *)mallocer(sizeof (library_t));
	if (*lib == NULL) {

		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	memset(*lib, 0, sizeof (library_t));

	/* scsi path is defined as upath_t in libary_t */
	strlcpy((*lib)->scsi_path, linkbuf, sizeof (upath_t));

	(*lib)->alternate_paths_list = lst_create();
	if ((*lib)->alternate_paths_list == NULL) {

		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	char *ptr = copystr(libpath);
	if (ptr == NULL) {
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	if (lst_append((*lib)->alternate_paths_list, ptr) != 0) {

		free(ptr);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	memset(&devdata, 0, sizeof (devdata_t));
	strlcpy(devdata.logdevpath, libpath, PATH_MAX + 1);

	if (_open_device(
	    libpath,
	    B_TRUE,		/* reconfig device using cfgadm */
	    &(devdata.fd)) != 0) {

		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "discover library by path - open device success");

	(*lib)->discover_state = DEV_READY;

	/* standard inquiry */
	if (_scsi_inq_std(
	    &devdata,
	    (*lib)->vendor_id,
	    (*lib)->product_id,
	    &((*lib)->scsi_version)) != 0) {

		close(devdata.fd);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	snprintf((*lib)->base_info.set, sizeof (uname_t), "%s-%s",
	    (*lib)->vendor_id, (*lib)->product_id);

	/* usn inquiry - page 80 */
	if (scsi_inq_pg80(&devdata, (*lib)->serial_no) != 0) {

		close(devdata.fd);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	/* read element status to get device */
	if (scsi_read_dev_element_status(
	    &devdata,
	    (*lib)->vendor_id,
	    (*lib)->product_id,
	    &drive_serialno_lst) != 0) {

		close(devdata.fd);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}
	close(devdata.fd);

	/* get equ_type of library from vendor id and product id */
	if (get_equ_type(
	    (*lib)->vendor_id,
	    (*lib)->product_id,
	    &supported_media, /* list of supported media is not input */
	    (*lib)->base_info.equ_type) != 0) {

		lst_free_deep(drive_serialno_lst);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	lst_free_deep_typed(supported_media, FREEFUNCCAST(free_medias_type));

	(*lib)->drive_list = lst_create();
	if ((*lib)->drive_list == NULL) {

		lst_free_deep(drive_serialno_lst);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "populate the drive information");
	/*
	 * library discovery only gets Drive Serial No.
	 * do drive discovery to populate the drive with
	 * vendor id, product id and other info
	 */
	node_p = drive_serialno_lst->head;
	while (node_p != NULL) {
		char *serial_no = (char *)node_p->data;
		sqm_lst_t *lst;
		drive_t *tdrive;

		if (discover_tape_drive(serial_no, NULL, &lst) != 0) {

			lst_free_deep(drive_serialno_lst);
			free_library(*lib);
			Trace(TR_ERR, "discover library by path failed: %s",
			    samerrmsg);
			return (-1);
		}
		if (lst->length == 0) {

			samerrno = SE_DRIVE_NOT_FOUND;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

			/* ignore and find the other drives of this lib */
			Trace(TR_ERR, "unable to discover drive:%s %s",
			    serial_no, samerrmsg);
		}
		for (tnode = lst->head; tnode != NULL; tnode = tnode->next) {

			tdrive = (drive_t *)tnode->data;
			if (tdrive == NULL) {
				continue;
			}

			strlcpy(tdrive->base_info.set,
			    (*lib)->base_info.set,
			    sizeof (tdrive->base_info.set));

			if (lst_append((*lib)->drive_list, tdrive) != 0) {

				lst_free_deep_typed(lst,
				    FREEFUNCCAST(free_drive));
				lst_free_deep(drive_serialno_lst);
				free_library(*lib);
				Trace(TR_ERR, "discover library by path failed:"
				    " %s", samerrmsg);
				return (-1);

			}
			/* set the node to null, so that it is not freed */
			tnode->data = NULL;
			// only one entry in list
			break;
		}
		lst_free_deep_typed(lst, FREEFUNCCAST(free_drive));
		lst = NULL;
		node_p = node_p->next;
	}
	/* If no drives are found, display error */
	if ((*lib)->drive_list == NULL || (*lib)->drive_list->length == 0) {

		samerrno = SE_DRIVE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		lst_free_deep(drive_serialno_lst);
		free_library(*lib);
		Trace(TR_ERR, "discover library by path failed: %s", samerrmsg);
		return (-1);
	}

	(*lib)->no_of_drives = drive_serialno_lst->length;
	(*lib)->media_license_list = NULL; /* obsolete */
	lst_free_deep(drive_serialno_lst);
	Trace(TR_OPRMSG, "discover library by path complete");

	return (0);
}


/*
 * discover a specific drive by path
 */
int
discover_standalone_drive_by_path(
ctx_t *ctx,		/* ARGSUSED */
drive_t **drive,	/* OUTPUT - drive_t */
upath_t drivepath)	/* INPUT - drive path */
{
	struct stat64 statbuf;
	node_t	*node = NULL;
	sqm_lst_t	*supported_media = NULL;
	sqm_lst_t	*id_lst;
	int	status;
	int	tape_flag = 0;
	int 	id_count = 0;
	int	i;
	devdata_t devdata;
	char	linkbuf[PATH_MAX+1];
	char	drivebuf[MAXPATHLEN+1];
	char	*bufp;
	char	*details[2];
	char	*last;
	char	*ptr;
	details[0] = NULL;
	details[1] = NULL;

	if (ISNULL(drive)) {
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}
	if (strlen(drivepath) == 0) {
		Trace(TR_ERR, "discover drive by path failed: %s",
		    "drive path is empty");
		return (-1);
	}

	Trace(TR_MISC, "discover drive by path %s", drivepath);

	status = lstat64(drivepath, &statbuf);
	if ((status == 0) && (S_ISLNK(statbuf.st_mode))) {
		status = readlink(drivepath, linkbuf, PATH_MAX + 1);
		linkbuf[status] = '\0'; /* readlink may not terminate */
	} else {
		strlcpy(linkbuf, drivepath, PATH_MAX + 1);
	}

	if (status < 0) {
		samerrno = SE_DEVICE_NOT_AVAILABLE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), drivepath);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	/* format here is blahblah/st@details0,details1 or *tape* */
	bufp = strstr(linkbuf, "st@");
	if (bufp == NULL) {
		bufp = strstr(linkbuf, "tape");
		if (bufp != NULL) {
			tape_flag++;
		}
	}

	if (bufp == NULL) {
		/* bad entry?? */
		samerrno = SE_DEVICE_NOT_AVAILABLE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), drivepath);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	/* scsi path */
	strlcpy(drivebuf, bufp, sizeof (drivebuf));
	*bufp = '\0';

	*drive = (drive_t *)mallocer(sizeof (drive_t));
	if (*drive == NULL) {
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}
	memset(*drive, 0, sizeof (drive_t));

	strlcpy((*drive)->scsi_path, linkbuf, sizeof (upath_t));

	bufp = drivebuf;
	for (i = 0; i < 2; i++) {
		details[i] = bufp;
		if (details[i] != NULL) {
			bufp = strchr(details[i], ',');
			if (bufp != NULL) {
				*bufp = '\0';
				bufp++;
			}
		}
	}

	/* get target id and lun id from parsing the device's physical path */
	if ((details[0] != NULL) && ((strlen(details[0]) > 10))) {
		if (!tape_flag) {
			bufp = &details[0][3];
		} else {
			bufp = &details[0][5];
		}
		for (i = 0; i < strlen(bufp); i++) {
			(*drive)->wwn_lun[i] = bufp[i];
		}
		bufp = strtok_r(details[1], ":", &last);
		if (bufp != NULL) {
			((*drive)->scsi_2_info).lun_id = -1;
			/* TBD: use strtol instead of atoi */
			((*drive)->scsi_2_info).target_id = atoi(bufp);
		}
	} else {
		((*drive)->scsi_2_info).target_id = atoi(&details[0][3]);
		if (details[1] != NULL) {
			bufp = strchr(details[1], ':');
			if (bufp != NULL) {
				*bufp = '\0';
			}
			((*drive)->scsi_2_info).lun_id = atoi(details[1]);
		}
	}

	Trace(TR_OPRMSG, "lunid=%d target =%d\n",
	    ((*drive)->scsi_2_info).lun_id,
	    ((*drive)->scsi_2_info).target_id);

	(*drive)->alternate_paths_list = lst_create();
	if ((*drive)->alternate_paths_list == NULL)  {

		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}
	ptr = copystr(drivepath);
	if (ptr == NULL) {

		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}
	if (lst_append((*drive)->alternate_paths_list, ptr) != 0) {

		free(ptr);
		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);

		return (-1);
	}

	memset(&devdata, 0, sizeof (devdata_t));
	strlcpy(devdata.logdevpath, drivepath, PATH_MAX + 1);

	if (_open_device(
	    drivepath,
	    B_FALSE, 	/* do not reconfigure using cfgadm */
	    &(devdata.fd)) != 0) {

		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	(*drive)->discover_state = DEV_READY;

	/* standard inquiry */
	if (_scsi_inq_std(
	    &devdata,
	    (*drive)->vendor_id,
	    (*drive)->product_id,
	    &((*drive)->scsi_version)) != 0) {

		close(devdata.fd);
		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	/* usn inquiry - page 80 */
	if (scsi_inq_pg80(&devdata, (*drive)->serial_no) != 0) {

		close(devdata.fd);
		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	if ((*drive)->scsi_version == 3) {
		/* TBD: how do you check if the device supports pg83 ? */

		if (_scsi_inq_pg83(&devdata, &id_lst) == 0) {

			/*
			 * the drive_t structure holds the devid as an array
			 * of wwn_id, each of a maximum length of WWN_LENGTH.
			 * The length of the array is MAXIMUM_WWN. Till the
			 * drive_t structure is modified, iterate through the
			 * id_lst and copy it into the wwn_id array
			 */
			node = id_lst->head;
			while (node != NULL && id_count < MAXIMUM_WWN) {
				devid_t *id = (devid_t *)node->data;

				(*drive)->wwn_id_type[id_count] = id->type;
				strlcpy((*drive)->wwn_id[id_count],
				    id->desc, WWN_LENGTH);

				id_count++;
				node = node->next;
			}
			lst_free_deep(id_lst);
		} else {
			Trace(TR_MISC, "Could not get scsi inq pg83 data");
		}
	}
	close(devdata.fd);

	/* get equ_type of library from vendor id and product id */
	if (get_equ_type(
	    (*drive)->vendor_id,
	    (*drive)->product_id,
	    &supported_media, /* list of supported media is not input */
	    (*drive)->base_info.equ_type) != 0) {

		free_drive(*drive);
		Trace(TR_ERR, "discover drive by path failed: %s", samerrmsg);
		return (-1);
	}

	lst_free_deep_typed(supported_media, FREEFUNCCAST(free_medias_type));

	Trace(TR_MISC, "discover drive by path success");
	return (0);
}


/*
 * discover all libraries
 *
 * Discover the libraries by traversing through all /dev/samst/ entries.
 * Issue a SCSI inquiry command to get drive identification. This method
 * also accepts an input filter to exclude the paths that are in mcf_paths
 * list. These are libraries that are already under SAM's control.
 *
 * A removable media can be accessed by multiple paths, so if there are
 * multiple entries in /dev/samst/ for the same removable media, do not create
 * multiple library_t, but just append the paths to the alternate path list in
 * library_t.
 */
int
discover_library(
sqm_lst_t *mcf_paths,	/* INPUT - paths to be excluded from discovery */
sqm_lst_t **lib_info_list)	/* OUTPUT - a list of library_t */
{
	void			*h_lib			= NULL;
	struct stat64		statbuf;
	struct dirent64		*entry			= NULL;
	struct dirent64		*entryp			= NULL;
	ssize_t			len			= 0;
	library_t		*my_lib			= NULL;
	int			ret			= -1;
	ht_iterator_t		*it			= NULL;
	hashtable_t		*ht_libs;
	DIR			*dirp			= NULL;
	char			libbuf[MAXPATHLEN+1]	= {0};
	char			pathbuf[MAXPATHLEN+1]	= {0};
	char			*bufp			= NULL;
	char			*samst_details		= NULL;
	int			i;

	Trace(TR_MISC, "discovering libraries in %s", SAMSTDIR);

	if (ISNULL(lib_info_list)) {
		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "discover library: process fc attached library");

	/*
	 * process cfgadm -al to get all fc-fabric attached library names. If
	 * the wwn of the library is not found in /kernel/drv/samst.conf,
	 * add the wwn to /kernel/drv/samst.conf.
	 */
	if (process_fc() != 0) {
		Trace(TR_MISC, "discover fc attached library failed: %s",
		    samerrmsg);
	}

	dirp = opendir(SAMSTDIR);
	if (dirp == NULL) {
		samerrno = SE_OPENDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), SAMSTDIR, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}

	entry = mallocer(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (entry == NULL) {

		closedir(dirp);
		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}

	ht_libs = ht_create();	/* for multiple paths to same lib */
	if (ht_libs == NULL) {
		free(entry);
		closedir(dirp);
		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}

	while ((ret = readdir64_r(dirp, entry, &entryp)) == 0) {
		if (entryp == NULL) {
			break;
		}

		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		snprintf(libbuf, sizeof (libbuf), "%s/%s",
		    SAMSTDIR, entry->d_name);

		ret = lstat64(libbuf, &statbuf);
		if (ret != 0) {
			/* skip bad entries */
			continue;
		}

		if (S_ISLNK(statbuf.st_mode)) {
			len = readlink(libbuf, pathbuf, sizeof (pathbuf));
			if (len == -1) {
				/* another bad entry */
				continue;
			}
			pathbuf[len] = '\0';	/* readlink may not terminate */
		} else {
			strlcpy(pathbuf, libbuf, sizeof (pathbuf));
		}

		/* if we're excluding mcf paths, check now */
		if (mcf_paths != NULL) {
			if (lst_search(mcf_paths, libbuf,
			    (lstsrch_t)strcmp) != NULL) {
				/* got this one already */
				continue;
			}
		}

		if (discover_library_by_path(NULL, &my_lib, libbuf) != 0) {

			Trace(TR_ERR, "discover library failed: %s", samerrmsg);
			/* ignore individual errors and find other libraries */
			continue;
		}

		/* format here is blahblah/samst@details */
		bufp = strstr(pathbuf, "samst@");
		if (bufp == NULL) {
			continue;
		}

		strlcpy(libbuf, bufp, sizeof (libbuf));
		*bufp = '\0';

		samst_details = libbuf;

		/* we're only interested in the first clause, lose the rest */
		if (samst_details != NULL) {
			bufp = strchr(samst_details, ',');
			if (bufp != NULL) {
				*bufp = '\0';
			}
		}


		/* look for the WWN */
		if ((samst_details != NULL) && (strlen(samst_details) > 10)) {
			bufp = &samst_details[6]; /* starts at the 7th char */
			for (i = 0; i < strlen(bufp); i++) {
				my_lib->wwn_lun[i] = bufp[i];
			}
		} else {
			strlcpy(my_lib->wwn_lun, "no",
			    sizeof (my_lib->wwn_lun));
		}

		/*
		 * check if this is alternate path to the same library
		 * already in h_libs
		 */

		if (ht_get(ht_libs, my_lib->serial_no, &h_lib) != 0) {

			free(entry);
			closedir(dirp);
			free_library(my_lib);
			ht_free_deep(&ht_libs, &free_library_void);
			Trace(TR_ERR, "discover library failed: %s", samerrmsg);
			return (-1);
		}

		if (h_lib == NULL) {
			char *key = copystr(my_lib->serial_no);
			/* key not found */
			if (ht_put(ht_libs, key, my_lib) != 0) {

				free(key);
				free_library(my_lib);
				ht_free_deep(&ht_libs, &free_library_void);
				free(entry);
				closedir(dirp);
				Trace(TR_ERR, "discover library failed: %s",
				    samerrmsg);
				return (-1);
			}
		} else { // multiple paths
			lst_concat(
			    ((library_t *)h_lib)->alternate_paths_list,
			    my_lib->alternate_paths_list);
			my_lib->alternate_paths_list = NULL;
			free_library(my_lib);
		}
	}

	free(entry);
	closedir(dirp);

	*lib_info_list = lst_create();
	if (*lib_info_list == NULL) {

		ht_free_deep(&ht_libs, &free_library_void);
		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}

	/* iterate through the hashtable and put it into a list */
	if (ht_get_iterator(ht_libs, &it) != 0) {

		lst_free(*lib_info_list);
		ht_free_deep(&ht_libs, &free_library_void);
		Trace(TR_ERR, "discover library failed: %s", samerrmsg);
		return (-1);
	}
	while (ht_has_next(it)) {
		char *key;
		void *value;
		if (ht_get_next(it, &key, &value) != 0) {

			free(it);
			lst_free(*lib_info_list);
			ht_free_deep(&ht_libs, &free_library_void);
			Trace(TR_ERR, "discover library failed: %s", samerrmsg);
			return (-1);
		}
		if (lst_append(*lib_info_list, value) != 0) {

			free(it);
			lst_free(*lib_info_list);
			ht_free_deep(&ht_libs, &free_library_void);
			Trace(TR_ERR, "discover library failed: %s", samerrmsg);
			return (-1);
		}
	}

	free(it);
	/* contents of the hashtable are in the output list */
	free_hashtable(&ht_libs);

	Trace(TR_OPRMSG, "finished discovering library information");
	return (0);

}


/*
 * Discover the tape drives by traversing through all /dev/rmt/ entries.
 * Issue a SCSI inquiry command to get drive identification. This method
 * also accepts 2 input filters, namely serial number and mcf paths.
 *
 * serial no:-
 * If a serial number is given, compare the serial number from the SCSI
 * INQUIRY with the input serial number. If a match is found, populate
 * the drive structure for that removable media. The output drive_lst
 * will have a single drive in the list.
 * If serial no is NULL, then return all the discovered drives that are
 * not already in SAM's control (mcf_paths)
 *
 * mcf_paths:-
 * If mcf_paths is given, exclude the paths that are in the mcf_paths list.
 * These are drives that are already under SAM's control.
 *
 * A removable media can be accessed by multiple paths, so if there are
 * multiple entries in /dev/rmt/ for the same removable media, do not create
 * multiple drive_t, but just append the paths to the alternate path list in
 * drive_t.
 */
int
discover_tape_drive(
char *serial_no,	/* Input filter - serial number to match */
sqm_lst_t *mcf_paths,	/* Input filter - list of paths to exclude */
sqm_lst_t **drive_lst)	/* Output - list of drive that match the serial */
			/* number, if none found; return empty list	*/
{
	drive_t			*my_drive;
	struct stat64		statbuf;
	struct dirent64		*entry = NULL;
	struct dirent64		*entryp;
	DIR			*dirp = NULL;
	int			ret;
	char			drivebuf[MAXPATHLEN+1];
	hashtable_t		*ht_drives = NULL;

	Trace(TR_OPRMSG, "discovering tape drive");

	if (ISNULL(drive_lst)) {
		return (-1);
	}

	dirp = opendir(TAPEDEVDIR);
	if (dirp == NULL) {
		samerrno = SE_OPENDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), TAPEDEVDIR, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "discover tape drives failed: %s", samerrmsg);
		return (-1);
	}

	entry = mallocer(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (entry == NULL) {

		closedir(dirp);
		Trace(TR_ERR, "discover tape drives failed: %s", samerrmsg);
		return (-1);
	}

	ht_drives = ht_create();
	if (ht_drives == NULL) {

		free(entry);
		closedir(dirp);
		Trace(TR_ERR, "discover tape drives failed: %s", samerrmsg);
		return (-1);
	}

	while ((ret = readdir64_r(dirp, entry, &entryp)) == 0) {
		if (entryp == NULL) {
			break;
		}

		if (fnmatch("*cbn", entry->d_name, 0) != 0) {
			continue;
		}

		snprintf(drivebuf, sizeof (drivebuf), "%s/%s", TAPEDEVDIR,
		    entry->d_name);

		ret = lstat64(drivebuf, &statbuf);
		if (ret != 0) {
			/* skip bad entries */
			continue;
		}

		/* if we're excluding mcf paths, check now */
		if (mcf_paths != NULL) {
			if (lst_search(mcf_paths, drivebuf,
			    (lstsrch_t)strcmp) != NULL) {
				/* got this one already */
				continue;
			}
		}

		if (discover_standalone_drive_by_path(
		    NULL, &my_drive, drivebuf) != 0) {
			if (samerrno == SE_DEVICE_NOT_AVAILABLE) {
				continue;
			}

			ht_free_deep(&ht_drives, &free_drive_void);
			free(entry);
			closedir(dirp);
			Trace(TR_ERR, "discover tape drives failed: %s",
				samerrmsg);
			return (-1);
		}

		/* If serial number is given, match against serial number */
		if (serial_no != NULL && serial_no[0] != '\0' &&
		    strncmp(serial_no, my_drive->serial_no,
			strlen(serial_no)) != 0) {

			free_drive(my_drive);
			my_drive = NULL;
			Trace(TR_MISC, "no match against given serial no");
			continue;
		}

		/*
		 * If serial number is given as input, a match against
		 * the serial number is found.
		 * If serial number is not given as inut, request is to
		 * get all the tape drives (excluding mcf paths)
		 *
		 * check if this is alternate path to the same drive
		 * already in h_drives
		 */
		void *h_drive;

		if (ht_get(ht_drives, my_drive->serial_no, &h_drive) != 0) {

			free_drive(my_drive);
			ht_free_deep(&ht_drives, &free_drive_void);
			free(entry);
			closedir(dirp);
			Trace(TR_ERR, "discover tape drives failed: %s",
				samerrmsg);
			return (-1);
		}
		if (h_drive == NULL) {
			/* key not found */
			char *key = copystr(my_drive->serial_no);

			if (ht_put(ht_drives, key, my_drive) != 0) {

				free(key);
				free_drive(my_drive);
				ht_free_deep(&ht_drives, &free_drive_void);
				free(entry);
				closedir(dirp);
				Trace(TR_ERR, "discover tape drives failed: %s",
					samerrmsg);
				return (-1);
			}
		} else { // multiple paths
			lst_concat(
				((drive_t *)h_drive)->alternate_paths_list,
				my_drive->alternate_paths_list);
			my_drive->alternate_paths_list = NULL;
			free_drive(my_drive);
		}
		break;
	}

	free(entry);
	closedir(dirp);

	*drive_lst = lst_create();
	if (*drive_lst == NULL) {
		return (-1);
	}
	/* get a list of drive_t from the ht_drives */
	if (serial_no != NULL && serial_no[0] != '\0') {
		drive_t *drive;
		if (ht_get(ht_drives, serial_no, (void **)&drive) != 0) {

			ht_free_deep(&ht_drives, &free_drive_void);
			lst_free(*drive_lst);
			Trace(TR_ERR, "discover tape drives failed: %s",
				samerrmsg);
			return (-1);
		}
		if (drive != NULL) {
			if (lst_append(*drive_lst, drive) != 0) {

				ht_free_deep(&ht_drives, &free_drive_void);
				lst_free(*drive_lst);
				Trace(TR_ERR, "discover tape drives failed: %s",
					samerrmsg);
				return (-1);
			}
			/* the content is in the output list */
			free_hashtable(&ht_drives);
		} else {
			ht_free_deep(&ht_drives, &free_drive_void);

		}
	} else {
		ht_iterator_t   *it = NULL;
		/* iterate through the hashtable and put it into a list */
		if (ht_get_iterator(ht_drives, &it) != 0) {

			ht_free_deep(&ht_drives, &free_drive_void);
			lst_free(*drive_lst);
			Trace(TR_ERR, "discover tape drives failed: %s",
				samerrmsg);
			return (-1);

		}
		while (ht_has_next(it)) {
			char *key;
			void *value;
			if (ht_get_next(it, &key, &value) != 0) {

				free(it);
				ht_free_deep(&ht_drives, &free_drive_void);
				lst_free(*drive_lst);

				Trace(TR_ERR, "discover tape drives failed: %s",
					samerrmsg);
				return (-1);
			}
			if (lst_append(*drive_lst, value) != 0) {

				free(it);
				ht_free_deep(&ht_drives, &free_drive_void);
				lst_free(*drive_lst);

				Trace(TR_ERR, "discover tape drives failed: %s",
					samerrmsg);
				return (-1);
			}
		}
		free(it);
		free_hashtable(&ht_drives);
	}

	Trace(TR_MISC, "discover tape drives complete");
	return (0);

}


/*
 * process cfgadm -al to get all fc-fabric attached library names. If the wwn
 * is not found in /kernel/drv/samst.conf, add the wwn to /kernel/drv/samst.conf
 * e.g.
 * name="samst" parent="fp" lun=0 fc-port-wwn="$wwn"
 *
 * If /kernel/drv/samst.conf does not exist, return error
 *
 */
static int
process_fc(void)
{
	struct stat64 statbuf;
	pid_t pid;
	node_t *node;
	sqm_lst_t *wwn_lst;
	int status, fd;
	FILE *fp = NULL;
	char *str, *wwn, *p, *last, *end_comment;
	char *delims = " ";
	char buf[BUFSIZ], new_samst_line[BUFSIZ];
	boolean_t added_samst_line = B_FALSE;
	boolean_t found = B_FALSE;

	Trace(TR_OPRMSG, "processing fabric attached library");

#if 0
	/* If /kernel/drv/samst.conf does not exist, return error */
	if (lstat64(SAMST_CFG, &statbuf) < 0) {

		samerrno = SE_SAMST_CONF_NOT_EXIST;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    SAMST_CFG);
		Trace(TR_ERR, "process fc-library failed: %s", samerrmsg);
		return (-1);
	}
#endif
	/* execute cfgadm and parse the command output to get WWN */
	pid = exec_get_output(MEDCHANGER_CFGADM_CMD, &fp, NULL);
	if (pid == -1) {

		Trace(TR_ERR, "process fc-library failed: %s", samerrmsg);
		return (-1);
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {

		fclose(fp);

		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    MEDCHANGER_CFGADM_CMD);
		Trace(TR_ERR, "process fc-library failed: %s", samerrmsg);
		return (-1);
	}

	wwn_lst = lst_create();
	if (wwn_lst == NULL) {

		fclose(fp);

		Trace(TR_ERR, "process fc-library failed: %s", samerrmsg);
		return (-1);
	}

	/* get wwns for any fc libs */
	while (fgets(buf, BUFSIZ, fp) != NULL) {

		str = &buf[0];

		/* tokenize on " " */
		p = strtok_r(str, delims, &last);
		while (p != NULL) {
			Trace(TR_OPRMSG, "token %s\n", p);
			p = strtok_r(NULL, delims, &last);
		}
		Trace(TR_DEBUG, "buf=%s.\n", str);
		strtok_r(str, "::", &last);
		Trace(TR_DEBUG, "last token is %s\n", last);

		if (strstr(last, "samst") != NULL) {
			Trace(TR_OPRMSG, "samst is mapped\n");

		} else {
			Trace(TR_OPRMSG, "wwn is mapped %s\n", &last[1]);

			str = copystr(&last[1]);
			if (str == NULL) {

				lst_free_deep(wwn_lst);
				fclose(fp);

				Trace(TR_ERR, "process fc-library failed: %s",
				    samerrmsg);
				return (-1);
			}
			if (lst_append(wwn_lst, str) != 0) {

				free(str);
				lst_free_deep(wwn_lst);
				fclose(fp);

				Trace(TR_ERR, "process fc-library failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}
	fclose(fp);
	fp = NULL;

	/* There is nothing to add so simply return */
	if (wwn_lst->length == 0) {
		lst_free_deep(wwn_lst);
		Trace(TR_OPRMSG, "no fabric attached libraries");
		return (0);
	}

	/*
	 * If WWNs do not exist in /kernel/drv/samst.conf, add an entry.
	 * The file should already exist.
	 */
#if 0
	fd = open(SAMST_CFG, O_RDWR);
	if (fd != -1) {
		fp = fdopen(fd, "r+");
	}

	if (fp == NULL) {

		lst_free_deep(wwn_lst);
		if (fd != -1) {
			close(fd);
		}
		samerrno = SE_FILE_APPEND_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    SAMST_CFG);

		Trace(TR_ERR, "process fc-library failed: %s", samerrmsg);
		return (-1);
	}

	/* For each wwn in the list look for it in samst.conf */
	for (node = wwn_lst->head; node != NULL; node = node->next) {

		found = B_FALSE;

		wwn = (char *)node->data;
		if (NULL == wwn) {
			continue;
		}
		snprintf(new_samst_line, sizeof (new_samst_line),
		    "name=\"samst\" parent=\"fp\" "
		    "lun=0 fc-port-wwn=\"%s\";\n",
		    wwn);

		while (fgets(buf, BUFSIZ, fp) != NULL) {

			if (buf[0] == '#') {
				continue;
			}

			end_comment = strchr(buf, '#');
			if (end_comment != NULL) {
				*end_comment = '\0';
			}


			/*
			 * search for the whole line or if that fails
			 * just for the wwn
			 */
			if (strncmp(buf, new_samst_line, strlen(buf)) == 0) {
				found = B_TRUE;
				break;
			} else if (strstr(buf, wwn) != NULL) {
				found = B_TRUE;
				break;
			}
		}

		if (!found) {
			boolean_t append_newline = B_FALSE;

			/* Verify the file currently ends with a newline */
			if (fseek(fp, -1, SEEK_END) != 0) {
				Trace(TR_ERR, "unable to seek end of %s",
				    SAMST_CFG);
				break;
			}

			if (fgetc(fp) != '\n') {
				append_newline = B_TRUE;
			}

			if (fseek(fp, 0, SEEK_END) != 0) {
				break;
			}

			if (append_newline) {
				fputc('\n', fp);
			}

			fputs(new_samst_line, fp);
			fflush(fp);
			Trace(TR_OPRMSG, "append %s to %s",
			    new_samst_line, SAMST_CFG);

			added_samst_line = B_TRUE;
		}
		/* Rewind for a possible second wwn */
		rewind(fp);
	}

	lst_free_deep(wwn_lst);
	fclose(fp);
	if (added_samst_line) {
		Trace(TR_OPRMSG, "process fc: run cmd:%s", DEVFSADM_CMD);
		pid = exec_get_output(DEVFSADM_CMD, NULL, NULL);
		if (pid == -1) {

			Trace(TR_ERR, "process fc: run devfsadm error: %s",
			    samerrmsg);
			return (-1);
		}

		if ((pid = waitpid(pid, &status, 0)) < 0) {

			samerrno = SE_FORK_EXEC_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    DEVFSADM_CMD);

			Trace(TR_ERR, "process fc: run devfsadm error: %s",
			    samerrmsg);
			return (-1);
		}

		/*
		 * Call to update the boot archive. If it fails simply log
		 * it because update-archive may not be supported on all
		 * hardware and all releases of the OS.
		 */
		Trace(TR_OPRMSG, "process fc: run cmd:%s", BOOTADM_CMD);
		pid = exec_get_output(BOOTADM_CMD, NULL, NULL);
		if (pid == -1) {

			Trace(TR_ERR, "process fc: run bootadm "
			    "update-archive error: %s", samerrmsg);
			return (0);
		}

		if ((pid = waitpid(pid, &status, 0)) < 0) {

			samerrno = SE_FORK_EXEC_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    BOOTADM_CMD);

			Trace(TR_ERR, "process fs: run bootadm update-archive "
			    "error: %s", samerrmsg);
			return (0);
		}
	}
#endif

	Trace(TR_OPRMSG, "finished processing fabric attached library");
	return (0);
}


static int
process_fc_lib(library_t *in_lib, char *wwn_lun) {
	node_t *node_lib;
	drive_t *print_p;
	char *str, *str1;
	char *lib_cmd = "/usr/sbin/cfgadm -alo show_FCP_dev \
		| /usr/bin/grep tape";
	char buf[BUFSIZ];
	char *p, *last;
	int status;
	pid_t pid;
	FILE *ptr;
	char *delims = " ";
	char	details[2][128];
	int i, j;
	int new_target[20];

	Trace(TR_OPRMSG, "processing fabric attached library");
/*
 *	begin library path process
 *
 */
	pid = exec_get_output(lib_cmd, &ptr, NULL);
	if (pid == -1) {
		Trace(TR_PROC, "%s", samerrmsg);
		goto error;
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FORK_EXEC_FAILED),
		    lib_cmd);
		Trace(TR_PROC, "%s", samerrmsg);
		fclose(ptr);
		goto error;
	}
	j = 0;
	while (fgets(buf, BUFSIZ, ptr) != NULL) {
		str = &buf[0];
		p = strtok_r(str, delims, &last);
		while (p != NULL) {
			Trace(TR_OPRMSG, "token %s\n", p);
			p = strtok_r(NULL, delims, &last);
		}

		Trace(TR_OPRMSG, "buf=%s.\n", str);
		strtok_r(str, "::", &last);
		Trace(TR_OPRMSG, "last token is %s\n", last);
		if (strstr(last, "samst") != NULL) {
			Trace(TR_OPRMSG,
				"samst is mapped\n");
		} else {
			Trace(TR_OPRMSG,
				"wwn is mapped %s\n", &last[1]);
			str1 = &last[1];
			p = strtok_r(str1, ",", &last);
			i = 0;
			while (p != NULL) {
				Trace(TR_OPRMSG, "token %s\n", p);
				strlcpy(details[i], p, sizeof (details[i]));
				p = strtok_r(NULL, ",", &last);
				i ++;
			}

			if (strcmp(details[0], &wwn_lun[1]) == 0) {
				new_target[j] = atoi(details[1]);
				j ++;
			}
		}
	}
	fclose(ptr);

	node_lib = (in_lib->drive_list)->head;
	i = 0;
	while (node_lib != NULL) {
		print_p = (drive_t *)node_lib->data;
		if ((print_p->scsi_2_info).target_id == 0) {
			(print_p->scsi_2_info).target_id = new_target[i];
		}
		i ++;
		node_lib = node_lib-> next;
	}
	Trace(TR_OPRMSG, "finished processing fabric attached library");
	return (0);
error:
	return (-1);
}


/*
 * samid_to_dt() returns the device_type (integer) when given the samfs id as
 * obtained from /etc/opt/SUNWsamfs/inquiry.conf. See man page inquiry.conf(4)
 * for more information.
 * If no match is found, UNDEFINED_SAM_DT (99) is returned
 */
int
samid_to_dt(
char *samid)	/* INPUT - samfs id */
{
	sam_model_t *sam_dev;

	for (sam_dev = sam_model; sam_dev->short_name != NULL; sam_dev++) {
		if (strcmp(sam_dev->short_name, samid) == 0) {
			return (sam_dev->dt);
		}
	}

	return (UNDEFINED_SAM_DT);
}


/*
 * read the network-attached library parameter file. The parameter file is the
 * interface between SAM-FS/SAM-QFS and the network-attached library software
 *
 * the parsing function is passed as an input with filename and buffer for
 * output results
 */
int
read_parameter_file(
char *path,	/* INPUT - path to parameter file */
int dt,		/* INPUT - device type - see devinfo.h */
void **param)	/* OUTPUT - */
{

	sqm_lst_t *strlst;
	int st;

	if (ISNULL(path, param)) {

		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}

	if (strlen(path) == 0) {

		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "read parameter file %s start", path);

	if (get_parameter_kv(path, &strlst) != 0) {

		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}

	switch (dt) {
		case DT_STKAPI:
			st = parse_stk_param(strlst, (stk_param_t **)param);
			break;
		case DT_IBMATL:
			st = parse_ibm3494_param(
			    strlst, (nwlib_base_param_t **)param);
			break;
		case DT_SONYPSC:
			st = parse_sony_param(
			    strlst, (nwlib_base_param_t **)param);
			break;
		case DT_GRAUACI:
			st = parse_adicgrau_param(
			    strlst, (nwlib_base_param_t **)param);
			break;
		default:
			samerrno = SE_UNSUPPORTED_LIBRARY;

			break;
	}
	lst_free_deep(strlst);

	if (st != 0) {

		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "read parameter file %s complete", path);
	return (0);
}


/*
 * parse the string list (key = value pairs) used in the StorageTek parameter
 * file. The keys are as follows:
 *
 * access		- user_id used by client for access control
 *
 * hostname		- hostname of the server running ACSLS
 *
 * ssihost		_ hostname of the multiphomed SAM-FS server on the lan
 *			connecting to the ACSLS host
 *
 * portnum		- port number for SSI services on the server that is
 *			running ACSLS, default is 50004
 *
 * ssi_net_port		- port number for incoming responses from the ACSLS svr
 *
 * csi_hostport		- port number to which the SSI will sends its
 *			ACSLS request on the ACSLS server
 *
 * capid		- Cartridge Access Port to be used for exporting VSNs
 *
 * capacity		- capacity of media supported by the StorageTek
 *
 * device_path_name	- path to the device on the client
 *
 */
int
parse_stk_param(
sqm_lst_t *strlst,		/* INPUT - list of strings */
stk_param_t **param)	/* OUTPUT - stk parameter */
{
	node_t *node;
	char *ptr1, *ptr2;
	int st = 0;
	char key[MAXPATHLEN];
	char val[MAXPATHLEN];

	if (ISNULL(strlst, param)) {
		Trace(TR_ERR, "parse stk param failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parse stk param");

	*param = (stk_param_t *)mallocer(sizeof (stk_param_t));
	if (*param == NULL) {

		Trace(TR_ERR, "parse stk param failed: %s", samerrmsg);
		return (-1);
	}
	memset(*param, 0, sizeof (stk_param_t));

	(*param)->stk_device_list = lst_create();
	if ((*param)->stk_device_list == NULL) {

		free_stk_param(*param);

		Trace(TR_ERR, "parse stk param failed: %s", samerrmsg);
		return (-1);
	}

	for (node = strlst->head; node != NULL; node = node->next) {

		key[0] = '\0';
		val[0] = '\0';

		ptr1 = (char *)node->data;
		if (ptr1 == NULL) {
			continue; /* ignore invalid entries */
		}
		ptr2 = strchr(ptr1, '=');
		if (ptr2 == NULL) {
			continue; /* ignore invalid entries */
		}

		*ptr2 = '\0';
		ptr2++;

		str_trim(ptr1, key);
		str_trim(ptr2, val);

		if (strlen(key) == 0 || strlen(val) == 0) {

			free_stk_param(*param);

			samerrno = SE_INVALID_PARAMETER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), ptr1);
			Trace(TR_ERR, "parse stk param failed: %s", samerrmsg);
			return (-1);
		}
		if (strncmp(key, "hostname", strlen(key)) == 0) {

			strlcpy((*param)->hostname, val, 32);

		} else if (strncmp(key, "access", strlen(key)) == 0) {

			strlcpy((*param)->access, val, 32);

		} else if (strncmp(key, "portnum", strlen(key)) == 0) {

			(*param)->portnum = strtol(val, (char **)NULL, 10);

		} else if (strncmp(key, "ssi_inet_portnum", strlen(key)) == 0) {

			(*param)->ssi_inet_portnum =
			    strtol(val, (char **)NULL, 10);

		} else if (strncmp(key, "csi_hostport", strlen(key)) == 0) {

			(*param)->csi_hostport = strtol(val, (char **)NULL, 10);

		} else if (strncmp(key, "ssihost", strlen(key)) == 0) {

			strlcpy((*param)->ssi_host, val, 32);

		} else if (strncmp(key, "capid", strlen(key)) == 0) {

			/* stk_cap is not a pointer in stk_param_t */
			st = parsekv_stk_CAP(val, &((*param)->stk_cap));

		} else if (strncmp(key, "capacity", strlen(key)) == 0) {

			st = parsekv_stk_capacity(
			    val, &((*param)->stk_capacity_list));

		/* device path always start at TAPEDEVDIR */
		} else if (strncmp(key, TAPEDEVDIR, strlen(TAPEDEVDIR)) == 0) {

			stk_device_t *dev;

			st = parsekv_stk_devicepaths(val, &dev);
			if (st == 0) {
				strlcpy(dev->pathname, key, sizeof (upath_t));

				if ((st = lst_append(
				    (*param)->stk_device_list, dev)) != 0) {

					free(dev);
					break;
				}
			}
		}

		if (st != 0) {

			free_stk_param(*param);

			Trace(TR_ERR, "parse stk param failed:%s", samerrmsg);
			return (-1);
		}
	}

	Trace(TR_OPRMSG, "parse stk param complete");
	return (0);
}


static parsekv_t stkcap_tokens[] = {
	{"acs",	offsetof(stk_cap_t, acs_num),	parsekv_int},
	{"lsm",	offsetof(stk_cap_t, lsm_num),	parsekv_int},
	{"cap",	offsetof(stk_cap_t, cap_num),	parsekv_int},
	{"",	0,				NULL}
};


/*
 * parse StorageTek Cartridge Access Port
 * This description starts with an open parenthesis followed by 3 key = value
 * pairs followed by a close parenthesis. The key = value pairs between the
 * parentheses may be separated by a comma (,), a colon (:) or by white space
 *
 */
static int
parsekv_stk_CAP(char *str, stk_cap_t *cap)
{
	char *buffer;
	char *start, *end;

	if (strlen(str) == 0) {
		return (0);	/* No string, done parsing */
	}

	Trace(TR_OPRMSG, "parse stk CAP parameter: %s", str);

	/* find the start of the kv pairs */
	start = strchr(str, '(');
	if (start == NULL) {

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk CAP failed: %s", samerrmsg);
		return (-1);
	}

	start++; /* skip the parentheses */
	buffer = copystr(start);
	if (buffer == NULL) {

		Trace(TR_ERR, "parse stk CAP failed: %s", samerrmsg);
		return (-1);
	}

	/* read up till the close parentheses */
	end = strrchr(buffer, ')');
	if (end == NULL) {

		free(buffer);

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk CAP failed: %s", samerrmsg);
		return (-1);
	}
	*end = '\0';

	/*
	 * parse the key values into tokens stk_cap_t. The  key = value pairs
	 * between the parentheses may be separated by a comma, a colon or by
	 * whitespace
	 *
	 * TBD: update parse_kv() to take delimiter as input argument
	 */
	if (parse_kv(buffer, &stkcap_tokens[0], (void *) cap) != 0) {

		/* unable to parse tokens */
		free(buffer);

		Trace(TR_ERR, "parse stk CAP failed: %s", samerrmsg);
		return (-1);
	}

	free(buffer);

	Trace(TR_OPRMSG, "parse stk CAP complete");
	return (0);

}

/*
 * parse StorageTek cartrige capacity parameter
 * This is a comma separated list of index = value pairs enclosed in
 * parentheses. index  is  the index into the media_type file (supplied
 * by StorageTek and  located  on  the  ACS system) and value is the
 * capacity of that media type in units of 1024 bytes. This is optional
 *
 */
int
parsekv_stk_capacity(char *str, sqm_lst_t **capacity_lst)
{
	char *buffer;
	char *start, *end;
	char *ptr1, *ptr2, *ptr3;
	int blanks;

	if (strlen(str) == 0) {

		Trace(TR_OPRMSG, "parse stk capacity complete - empty string");
		return (0);
	}

	Trace(TR_OPRMSG, "parse stk capacity parameter: %s", str);

	if (ISNULL(capacity_lst)) {

		Trace(TR_ERR, "parse stk capacity failed: %s", samerrmsg);
		return (-1);
	}

	/* find the start of the kv pairs */
	start = strchr(str, '(');
	if (start == NULL) {

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk capacity failed: %s", samerrmsg);
		return (-1);
	}
	start++; /* skip the open parenthesis */

	buffer = copystr(start);
	if (buffer == NULL) {

		Trace(TR_ERR, "parse stk capacity failed: %s", samerrmsg);
		return (-1);
	}

	/* read up till the close parentheses */
	end = strrchr(buffer, ')');
	if (end == NULL) {

		free(buffer);

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk capacity failed: %s", samerrmsg);
		return (-1);
	}
	*end = '\0';

	*capacity_lst = lst_create();
	if (*capacity_lst == NULL) {

		free(buffer);

		Trace(TR_ERR, "parse stk capacity failed: %s", samerrmsg);
		return (-1);
	}

	ptr1 = buffer;
	while (ptr1 != NULL) {
		blanks = strspn(ptr1, WHITESPACE);
		ptr1 += blanks; /* Skip over whitespace */

		/* find next keyword */
		ptr2 = strchr(ptr1, ','); /* comma delimited */
		if (ptr2 != NULL) {
			*ptr2 = 0;
			ptr2++;
		}

		/* find the index and value within the index = value pair */
		ptr3 = strchr(ptr1, '=');
		if (ptr3 != NULL) {
			*ptr3 = 0;
			ptr3++;

			stk_capacity_t *cap = mallocer(sizeof (stk_capacity_t));
			if (cap == NULL) {

				lst_free_deep(*capacity_lst);
				free(buffer);

				Trace(TR_ERR, "parse stk capacity failed: %s",
				    samerrmsg);
				return (-1);
			}

			cap->index = strtol(ptr1, (char **)NULL, 10);
			cap->value = strtol(ptr3, (char **)NULL, 10);

			if (lst_append(*capacity_lst, cap) != 0) {

				free(cap);
				lst_free_deep(*capacity_lst);
				free(buffer);

				Trace(TR_ERR, "parse stk capacity failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
		ptr1 = ptr2;
	}

	free(buffer);

	Trace(TR_OPRMSG, "parse stk capacity complete");
	return (0);

}


static parsekv_t stkdevice_tokens[] = {
	{"acs",		offsetof(stk_device_t, acs_num),	parsekv_int},
	{"lsm",		offsetof(stk_device_t, lsm_num),	parsekv_int},
	{"panel",	offsetof(stk_device_t, panel_num),	parsekv_int},
	{"drive",	offsetof(stk_device_t, drive_num),	parsekv_int},
	{"",		0,					NULL}
};


/*
 * parse the device_path_name entry. This is the path to the device on the
 * client that is connected to the StorageTek drive
 *
 * This description starts with an open parenthesis followed by 4 key = value
 * pairs followed by a close parenthesis. The key-value pairs between the
 * parentheses may be separated by a comma, a colon or by white space.
 *
 * The keyword identifiers  and  their  meanings are as follows:
 * acs	- ACS number for this drive as configured in the StorageTek library
 * lsm	- LSM number for this drive as configured in the StorageTek library
 * panel- PANEL number for this drive as configured in the StorageTek library
 * drive- DRIVE number for this drive as configured in the StorageTek library
 *
 * Optional - After the close parenthesis, the presence of the 'shared' word
 * indicates that this drive is shared with other SAM-FS servers.
 */
static int
parsekv_stk_devicepaths(char *str, stk_device_t **dev)
{
	int blanks;
	char *buffer;
	char *start, *end;

	if (strlen(str) == 0) {

		Trace(TR_OPRMSG, "parse stk device path complete - empty str");
		return (0); /* no parsing required */
	}

	Trace(TR_OPRMSG, "parse stk device path name: %s", str);

	if (ISNULL(dev)) {

		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}

	start = strchr(str, '(');	/* skip till the first parenthesis */
	if (start == NULL) {

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}
	start++; /* skip the open parenthesis */

	buffer = copystr(start);
	if (buffer == NULL) {

		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}

	end = strrchr(buffer, ')');
	if (end == NULL) {

		free(buffer);

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), str);
		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}
	*end = '\0';
	end++; /* optional shared keyword */

	/* parse the key values into tokens stk_device_t */
	*dev = mallocer(sizeof (stk_device_t));
	if (*dev == NULL) {

		free(buffer);

		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}
	memset(*dev, 0, sizeof (stk_device_t));

	/* TBD: parse_kv should take DELIMITER as an input argument */
	if (parse_kv(buffer, &stkdevice_tokens[0], *dev) != 0) {

		free(dev);
		free(buffer);

		Trace(TR_ERR, "parse stk device path failed: %s", samerrmsg);
		return (-1);
	}

	if (strlen(end) > 0) {
		blanks = strspn(end, WHITESPACE);
		end += blanks; /* Skip over whitespace */
		if (strncmp(end, "shared", strlen(end)) == 0) {
			(*dev)->shared = B_TRUE;
		}
	}

	free(buffer);

	Trace(TR_OPRMSG, "parse stk device path complete");
	return (0);
}


/*
 * parse the list of strings (key = value pairs) used in the Sony library
 * parameter file. The parameter file is validated for required params
 *
 * userid	- userid (required parameter)
 *		0 <= userid <= 65535 (decimal)
 *		0 <= userid <= 0xffff (hexadecimal)
 *
 * server	- specifies the host name of the server running PSC server code
 *		(this is a required parameter)
 *
 * sonydrive	- binnum = path [ shared ]
 *		binnum 	- Specifies the bin number assigned to the drive in PSC
 *		path	- Specifies the Solaris /dev/rmt/ path to the device
 *		shared	- optional keyword indicating that the drive is shared
 *
 * The userid and binnum are ignored
 */
int
parse_sony_param(
sqm_lst_t *strlst,
nwlib_base_param_t **param)
{
	node_t *node;
	char *ptr1, *ptr2;
	char *last;
	char key[MAXPATHLEN];
	char val[MAXPATHLEN];

	if (ISNULL(strlst, param)) {

		Trace(TR_ERR, "parse sony param failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parsing sony param");

	*param = (nwlib_base_param_t *)mallocer(sizeof (nwlib_base_param_t));
	if (*param == NULL) {

		Trace(TR_ERR, "parse sony param failed: %s", samerrmsg);
		return (-1);
	}
	memset(*param, 0, sizeof (nwlib_base_param_t));

	(*param)->drive_lst = lst_create();
	if ((*param)->drive_lst == NULL) {

		free_nwlib_base_param(*param);

		Trace(TR_ERR, "parse sony param failed: %s", samerrmsg);
		return (-1);
	}

	for (node = strlst->head; node != NULL; node = node->next) {

		ptr1 = (char *)node->data;
		if (ptr1 == NULL) {
			continue;
		}

		ptr2 = strchr(ptr1, '=');
		if (ptr2 == NULL) {	/* invalid line */
			continue;
		}

		*ptr2 = '\0';
		ptr2++;

		str_trim(ptr1, key);
		str_trim(ptr2, val);

		if (strlen(key) == 0 || strlen(val) == 0) {

			free_nwlib_base_param(*param);

			samerrno = SE_INVALID_PARAMETER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), ptr1);
			Trace(TR_ERR, "parse sony param failed: %s", samerrmsg);
			return (-1);
		}

		if (strncmp(key, "server", strlen(key)) == 0) {

			strlcpy((*param)->server, val, 32);

		/* sonydrive bin_number = /dev/rmt/ */
		} else if (strstr(key, "sonydrive") != NULL) {

			/* val should of the form /dev/rmt/XXX [shared] */
			if (strncmp(val, TAPEDEVDIR, strlen(TAPEDEVDIR)) != 0) {

				free_nwlib_base_param(*param);

				samerrno = SE_INVALID_PARAMETER;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), val);
				Trace(TR_ERR, "parse sony param failed: %s",
				    samerrmsg);
				return (-1);
			}

			drive_param_t *drive = mallocer(sizeof (drive_param_t));
			if (drive == NULL) {

				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse sony param failed: %s",
				    samerrmsg);
				return (-1);
			}

			/* split val to check if shared keyword is present */

			/* ptr1 => dev path, last => shared (optional) */
			ptr1 = strtok_r(val, WHITESPACE, &last);
			if (ptr1 != NULL) {
				strlcpy(drive->path, ptr1, MAXPATHLEN);

				ptr1 = strtok_r(NULL, WHITESPACE, &last);
			}
			if ((ptr1 != NULL) &&
			    (strncmp(ptr1, "shared", strlen(ptr2)) == 0)) {

				drive->shared = B_TRUE;
			}

			if (lst_append((*param)->drive_lst, drive) != 0) {

				free(drive);
				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse sony param failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_OPRMSG, "parse sony parameter file complete");
	return (0);
}


/*
 * parse the list of strings (key = value pairs) used in the IBM3494
 * network-attached tape library parameter file. The key we look for are:
 *
 * name			- symbolic name of the library in /etc/ibmatl.conf
 * category		- hex number between 0x0001 and 0xfeff
 * access		- access to the library is shared or private
 * device-path-name	- drive path and device number and shared (optional)
 * The access and category keys and values are ignored
 *
 */
int
parse_ibm3494_param(
sqm_lst_t *strlst,
nwlib_base_param_t **param)
{
	node_t *node;
	char *ptr1, *ptr2;
	char *last;
	char key[MAXPATHLEN];
	char val[MAXPATHLEN];

	if (ISNULL(strlst, param)) {

		Trace(TR_ERR, "parse ibm param failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parsing ibm param");

	*param = (nwlib_base_param_t *)mallocer(sizeof (nwlib_base_param_t));
	if (*param == NULL) {

		Trace(TR_ERR, "parse ibm param failed: %s", samerrmsg);
		return (-1);
	}
	memset(*param, 0, sizeof (nwlib_base_param_t));

	(*param)->drive_lst = lst_create();
	if ((*param)->drive_lst == NULL) {

		free_nwlib_base_param(*param);

		Trace(TR_ERR, "parse ibm param failed: %s", samerrmsg);
		return (-1);
	}

	for (node = strlst->head; node != NULL; node = node->next) {

		ptr1 = (char *)node->data;
		if (ptr1 == NULL) { /* invalid line */
			continue;
		}
		ptr2 = strchr(ptr1, '=');
		if (ptr2 == NULL) {	/* invalid line */
			continue;
		}

		*ptr2 = '\0';
		ptr2++;

		str_trim(ptr1, key);
		str_trim(ptr2, val);

		if (strlen(key) == 0 || strlen(val) == 0) {

			free_nwlib_base_param(*param);

			samerrno = SE_INVALID_PARAMETER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), ptr1);
			Trace(TR_ERR, "parse ibm param failed: %s", samerrmsg);
			return (-1);
		}

		/* interested in name and drive entries */
		if (strncmp(key, "name", strlen(key)) == 0) {

			strlcpy((*param)->server, val, 32);

		/* device path always start at TAPEDEVDIR */
		} else if (strncmp(key, TAPEDEVDIR, strlen(TAPEDEVDIR)) == 0) {

			drive_param_t *drive = mallocer(sizeof (drive_param_t));
			if (drive == NULL) {

				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse ibm param failed: %s",
				    samerrmsg);
				return (-1);
			}

			strlcpy(drive->path, key, MAXPATHLEN);

			/* path = device number [shared] */

			ptr1 = strtok_r(val, WHITESPACE, &last);
			if (ptr1 != NULL) {
				ptr1 = strtok_r(NULL, WHITESPACE, &last);
			}
			if ((ptr1 != NULL) &&
			    (strncmp(ptr1, "shared", strlen(ptr2)) == 0)) {

				drive->shared = B_TRUE;
			}

			if (lst_append((*param)->drive_lst, drive) != 0) {

				free(drive);
				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse ibm param failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_OPRMSG, "parse ibm3494 param complete");
	return (0);
}


/*
 * parse the list of string s (key = value pairs) used in the ADIC/Grau
 * network-attached library.
 *
 * server	- hostname of the server running the DAS server code
 * acidrive	- one entry for every drive assigned to this client
 * The client key and value are ignored.
 */
int
parse_adicgrau_param(
sqm_lst_t *strlst,
nwlib_base_param_t **param)
{
	node_t *node;
	char key[MAXPATHLEN];
	char val[MAXPATHLEN];
	char *ptr1, *ptr2;

	if (ISNULL(strlst, param)) {

		Trace(TR_ERR, "parse grau param failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parse grau param");

	*param = (nwlib_base_param_t *)mallocer(sizeof (nwlib_base_param_t));
	if (*param == NULL) {

		Trace(TR_ERR, "parse grau param failed: %s", samerrmsg);
		return (-1);
	}
	memset(*param, 0, sizeof (nwlib_base_param_t));

	(*param)->drive_lst = lst_create();
	if ((*param)->drive_lst == NULL) {

		free_nwlib_base_param(*param);

		Trace(TR_ERR, "parse grau param failed: %s", samerrmsg);
		return (-1);
	}

	for (node = strlst->head; node != NULL; node = node->next) {

		ptr1 = (char *)node->data;
		if (ptr1 == NULL) {	/* invalid line */
			continue;
		}

		ptr2 = strchr(ptr1, '=');
		if (ptr2 == NULL) {	/* invalid line */
			continue;
		}

		*ptr2 = '\0';
		ptr2++;

		str_trim(ptr1, key);
		str_trim(ptr2, val);

		if (strlen(key) == 0 || strlen(val) == 0) {

			free_nwlib_base_param(*param);

			samerrno = SE_INVALID_PARAMETER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), ptr1);
			Trace(TR_ERR, "parse grau param failed: %s", samerrmsg);
			return (-1);
		}

		if (strncmp(key, "server", strlen(key)) == 0) {

			strlcpy((*param)->server, val, 32);
		/* acidrive drivename = /dev/rmt/ */
		} else if (strstr(key, "acidrive") != NULL) {

			drive_param_t *drive = mallocer(sizeof (drive_param_t));
			if (drive == NULL) {

				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse grau param failed: %s",
				    samerrmsg);
				return (-1);
			}
			memset(drive, 0, sizeof (drive_param_t));

			strlcpy(drive->path, val, MAXPATHLEN);

			if (lst_append((*param)->drive_lst, drive) != 0) {

				free(drive);
				free_nwlib_base_param(*param);

				Trace(TR_ERR, "parse grau param failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_OPRMSG, "parse fujitsu grau complete");
	return (0);
}


/*
 * read the parameter file for the Network-attached library. The path to the
 * parameter file is provided as input.
 * e.g. path = /etc/opt/SUNWsamfs/stk180 - StorageTek ACSLS library
 * The parameter file consists of keyword = value pairs and path/drive_name =
 * value pairs. These are saved as list of strings, each string is a key = value
 *
 */
static int
get_parameter_kv(
char	*path,		/* INPUT - path to parameter file */
sqm_lst_t	**strlst) 	/* OUTPUT - parameter as a list of strings */
{
	FILE *fp;
	char *ptr, *str;
	char *end_comment;
	char line[BUFSIZ];

	if (ISNULL(strlst)) {
		Trace(TR_ERR, "get parameter key=value failed: %s", samerrmsg);
		return (-1);
	}

	if (strlen(path) == 0) {

		samerrno = SE_INVALID_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), path);
		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "reading parameter file[%s]", path);

	if ((fp = fopen(path, "r")) == NULL) {

		samerrno = SE_FILE_READ_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), path);
		Trace(TR_ERR, "read parameter file failed: %s", samerrmsg);
		return (-1);
	}


	*strlst = lst_create();
	if (*strlst == NULL) {

		fclose(fp);
		Trace(TR_ERR, "get parameter key=value failed: %s", samerrmsg);
		return (-1);
	}

	while (fgets(line, BUFSIZ, fp) != NULL) {

		/* If comment, continue to next line */
		if (line[0] == '#')  {
			continue;
		}
		end_comment = strchr(line, '#');
		if (end_comment != NULL) {
			*end_comment = '\0';
		}
		ptr = strchr(line, '=');
		if (ptr == NULL) {	/* invalid line */
			continue;
		}

		str = copystr(line);
		if (str == NULL) {

			lst_free_deep(*strlst);
			fclose(fp);

			Trace(TR_ERR, "parse parameter file failed: %s",
			    samerrmsg);
			return (-1);
		}

		if (lst_append(*strlst, str) != 0) {

			free(str);
			lst_free_deep(*strlst);
			fclose(fp);

			Trace(TR_ERR, "get parameter key=value failed: %s",
			    samerrmsg);
			return (-1);
		}
	}
	fclose(fp);

	Trace(TR_OPRMSG, "get parameter key=value from file complete");
	return (0);
}


/*
 *	verify_library()
 *	given a library_path, we do discovery and then
 *	compare with mcf library.
 */
int
verify_library(
library_t *mcf_lib)	/* library_t structure based on MCF */
{
	node_t *node_tape = NULL;
	node_t *node_type;
	drive_t *print_tape;
	library_t *my_lib;
	drive_t *print_discover_tape;

	Trace(TR_OPRMSG, "verifying MCF's library setup");
	if (discover_library_by_path(NULL, &my_lib,
	    mcf_lib->base_info.name) == -1) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	compare my_lib with given lib.
	 */
	if (strcmp(mcf_lib->base_info.equ_type, "rb") == 0 &&
	    strcmp(my_lib->base_info.equ_type, UNDEFINED_EQU_TYPE) != 0) {
		strlcpy(mcf_lib->base_info.equ_type, my_lib->base_info.equ_type,
		    sizeof (mcf_lib->base_info.equ_type));
	} else if (strcmp(mcf_lib->base_info.equ_type,
	    my_lib->base_info.equ_type) != 0) {
		samerrno = SE_WRONG_ROBOT_TYPE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), mcf_lib->base_info.equ_type);
		Trace(TR_OPRMSG, "%s", samerrmsg);
		goto error;
	}

	strlcpy(mcf_lib->vendor_id, my_lib->vendor_id,
	    sizeof (mcf_lib->vendor_id));
	strlcpy(mcf_lib->product_id, my_lib->product_id,
	    sizeof (mcf_lib->product_id));
	strlcpy(mcf_lib->serial_no, my_lib->serial_no,
	    sizeof (mcf_lib->serial_no));

	/*
	 *	check the path of device inside library
	 */
	if ((mcf_lib->drive_list == NULL) ||
	    ((mcf_lib->drive_list)->length == 0)) {
		/* nothing to do */
		free_library(my_lib);
		Trace(TR_OPRMSG, "finished verifying MCF's library setup");
		return (0);
	}

	node_tape = mcf_lib->drive_list->head;

	while (node_tape != NULL) {
		print_tape = (drive_t *)node_tape->data;
		node_type = my_lib->drive_list->head;
		while (node_type != NULL) {
			print_discover_tape = (drive_t *)node_type->data;
			if (lst_search(
			    print_discover_tape->alternate_paths_list,
			    print_tape->base_info.name,
			    (lstsrch_t)strcmp)
			    != NULL) {
				node_type = node_type->next;
				continue;
			}
			/* check media type */
			if (strcmp(
			    print_tape-> base_info.equ_type, "tp") == 0 ||
			    strcmp(
			    print_tape-> base_info.equ_type, "op") == 0) {

				strlcpy(print_tape-> base_info.equ_type,
				    print_discover_tape-> base_info.equ_type,
				    sizeof (print_tape-> base_info.equ_type));
			} else if (strcmp(print_tape-> base_info.equ_type,
			    print_discover_tape-> base_info.equ_type) != 0) {
				samerrno = SE_WRONG_MEDIA_TYPE;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno),
				    print_tape->base_info.  equ_type);
				Trace(TR_OPRMSG, "%s", samerrmsg);
				goto error;
			}
			strlcpy(print_tape->vendor_id,
			    print_discover_tape->vendor_id,
			    sizeof (print_tape->vendor_id));
			strlcpy(print_tape->product_id,
			    print_discover_tape->product_id,
			    sizeof (print_tape->product_id));
			strlcpy(print_tape->serial_no,
			    print_discover_tape->serial_no,
			    sizeof (print_tape->serial_no));

			node_type = node_type->next;
		}
		node_tape = node_tape->next;
	}

	free_library(my_lib);
	Trace(TR_OPRMSG, "finished verifying MCF's library setup");
	return (0);
error:
	free_library(my_lib);
	return (-1);
}


/*
 *	verify_standalone_drive()
 *	given a standalone drive's path, we do discovery and then
 *	compare with mcf standalone drive.
 */
int
verify_standalone_drive(
drive_t *mcf_standalone_drive)	/* drive_t structure based on MCF */
{
	drive_t *my_drive;

	Trace(TR_OPRMSG, "verifying standalone drive");

	if (discover_standalone_drive_by_path(NULL, &my_drive,
	    mcf_standalone_drive->base_info.name) == -1) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	compare my_drive with MCF's standalone drive.
	 */
	if (strcmp(mcf_standalone_drive->base_info.equ_type, "tp") == 0 &&
	    strcmp(my_drive->base_info.equ_type, UNDEFINED_EQU_TYPE) != 0) {
		strlcpy(mcf_standalone_drive->base_info.equ_type,
		    my_drive->base_info.equ_type,
		    sizeof (mcf_standalone_drive->base_info.equ_type));
	} else if (strcmp(mcf_standalone_drive->base_info.equ_type,
	    my_drive->base_info.equ_type) != 0) {
		samerrno = SE_WRONG_MEDIA_TYPE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno),
		    mcf_standalone_drive->base_info.equ_type);
		Trace(TR_OPRMSG, "%s", samerrmsg);
		free_drive(my_drive);
		return (-1);
	}

	free_drive(my_drive);
	Trace(TR_OPRMSG, "finished verifying standalone drive");
	return (0);
}


/*
 * write STK ACSLS parameters to the given file path.
 */
int
write_stk_param(
stk_param_t *stk_param,	/* INPUT - STK parameters */
char *path)		/* INPUT - path to file */
{
	stk_device_t	*dev;
	stk_capacity_t	*cap;
	node_t		*n;
	int		fd, i;
	FILE		*fp = NULL;

	if (ISNULL(stk_param, path)) {
		Trace(TR_ERR, "write STK ACSLS parameters failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "writing STK ACSLS parameters to file %s", path);

	/*
	 * The capacity entry in stk the parameter file is optional and only
	 * required if the ACS is not returning the correct media type or if
	 * new media types that are not yet recognised by ACSLS have been added.
	 * The correct way to get this capacity would be from the user.
	 *
	 * The device entries are required
	 */
	if (stk_param->stk_device_list == NULL) {

		samerrno = SE_ACSLS_DEVICE_LIST_NULL;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "write STK ACSLS parameters failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* if file_path exists, update it. Keep a backup */
	if (backup_cfg(path) != 0) {
		Trace(TR_ERR, "write STK ACSLS parameters failed: %s",
		    samerrmsg);
		return (-1);
	}

	fd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	if (fd != -1) {
		fp = fdopen(fd, "w");
	}

	if (fp == NULL) {

		samerrno = SE_FILE_APPEND_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), path);

		Trace(TR_ERR, "write STK ACSLS parameters failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strlen(stk_param->access) > 0) {
		fprintf(fp, "access = %s\n", stk_param->access);
	}
	fprintf(fp, "hostname = %s\n", stk_param->hostname);
	fprintf(fp, "portnum = %d\n", stk_param->portnum);
	if (strlen(stk_param->ssi_host) > 0) {
		fprintf(fp, "ssihost = %s\n", stk_param->ssi_host);
	}
	if (stk_param->ssi_inet_portnum != -1) {
		fprintf(fp, "ssi_inet_portnum = %d\n",
		    stk_param->ssi_inet_portnum);
	}
	if (stk_param->csi_hostport != -1) {
		fprintf(fp, "csi_hostport = %d\n", stk_param->csi_hostport);
	}
	fprintf(fp, "capid = (acs=%d, lsm=%d, cap=%d)\n",
	    stk_param->stk_cap.acs_num, stk_param->stk_cap.lsm_num,
	    stk_param->stk_cap.cap_num);

	if (stk_param->stk_capacity_list != NULL &&
	    stk_param->stk_capacity_list->length > 0) {

		fprintf(fp, "capacity = ( ");
		for (i = 0, n = stk_param->stk_capacity_list->head; n != NULL;
		    n = n->next, i++) {

			cap = (stk_capacity_t *)n->data;
			if (i) {
				fprintf(fp, ", ");
			}
			fprintf(fp, "%d = %llu ", cap->index, cap->value);
		}
		fprintf(fp, " )\n");
	}

	for (n = stk_param->stk_device_list->head; n != NULL; n = n->next) {

		dev = (stk_device_t *)n->data;
		fprintf(fp, "%s = (acs=%d, lsm=%d, panel=%d, drive=%d) %s\n",
		    dev->pathname,
		    dev->acs_num,
		    dev->lsm_num,
		    dev->panel_num,
		    dev->drive_num,
		    (dev->shared == B_TRUE ? "shared" : " "));
	}
	fclose(fp);

	Trace(TR_OPRMSG, "write STK ACSLS parameters complete");
	return (0);
}


/*
 *	remove stk's ACSLS parameter file given path.
 *	We move the original file to a backup file.
 */
int
remove_stk_param(
char *file_path)		/* STK library parameter file path */
{
	struct stat64 path_stat;

	Trace(TR_OPRMSG, "remove stk parameter file: %s", file_path);

	/*
	 * if file_path exist, move it to a backup file.
	 */
	if (lstat64(file_path, &path_stat) == 0) {
		if (backup_cfg(file_path) != 0) {
			Trace(TR_OPRMSG, "%s", samerrmsg);
			return (-1);
		}
		unlink(file_path);
	}

	Trace(TR_OPRMSG, "finished removing stk parameter file");
	return (0);
}


/*
 *	write stk's ACSLS parameter file given path.
 *
 *  WHY do we need both this and write_stk_params?  This seems a
 *  mere superset of the other.
 */
int
update_stk_param(
stk_param_t *stk_parameter,	/* given the stk parameter structure */
char *file_path,		/* STK library parameter file path */
char *drive_path,		/* STK library parameter file path */
boolean_t shared_state)		/* STK library parameter file path */
{
	int ii;
	struct stat64 path_stat;
	FILE *file_ptr = NULL;
	node_t *node_cap;
	stk_capacity_t *stk_capacity;
	stk_device_t *stk_dev_path;
	node_t *node_path;
	int fd;

	Trace(TR_OPRMSG, "updating stk parameter file to %s", file_path);

	/*
	 * validate required parameters before changing anything
	 */

	if (stk_parameter->stk_capacity_list == NULL) {
		samerrno = SE_ACSLS_CAPACITY_LIST_NULL;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACSLS_CAPACITY_LIST_NULL));
		Trace(TR_ERR, " capacity list NULL\n");
		return (-1);
	}

	if (stk_parameter->stk_device_list == NULL) {
		samerrno = SE_ACSLS_DEVICE_LIST_NULL;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACSLS_DEVICE_LIST_NULL));
		Trace(TR_ERR, " device list NULL\n");
		return (-1);
	}

	/*
	 * if file_path exists, copy it to a backup file.
	 * and then overwrite it.
	 */
	if (lstat64(file_path, &path_stat) == 0) {
		if (backup_cfg(file_path) != 0) {
			Trace(TR_OPRMSG, "%s", samerrmsg);
			return (-1);
		}
	}

	fd = open(file_path, O_WRONLY|O_APPEND|O_CREAT, 0644);
	if (fd != -1) {
		file_ptr = fdopen(fd, "a");
	}

	if (file_ptr == NULL) {
		samerrno = SE_FILE_APPEND_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_APPEND_OPEN_FAILED),
		    file_path);
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "access %s\n", stk_parameter->access);
	Trace(TR_OPRMSG, "hostname = %s\n", stk_parameter->hostname);
	Trace(TR_OPRMSG, "portnum = %d\n", stk_parameter->portnum);
	Trace(TR_OPRMSG, "ssi_host = %s\n", stk_parameter->ssi_host);
	Trace(TR_OPRMSG, "capid = (acs=%d, lsm=%d, cap=%d)\n",
	    stk_parameter->stk_cap.acs_num, stk_parameter->stk_cap.lsm_num,
	    stk_parameter->stk_cap.cap_num);

	if (strlen(stk_parameter->access) > 0) {
		fprintf(file_ptr, "access = %s\n", stk_parameter->access);
	}
	fprintf(file_ptr, "hostname = %s\n", stk_parameter->hostname);
	fprintf(file_ptr, "portnum = %d\n", stk_parameter->portnum);
	if (strlen(stk_parameter->ssi_host) > 0) {
		fprintf(file_ptr, "ssihost = %s\n", stk_parameter->ssi_host);
	}
	if (stk_parameter->ssi_inet_portnum != -1 &&
	    stk_parameter->ssi_inet_portnum != 0) {
		fprintf(file_ptr, "ssi_inet_portnum = %d\n",
		    stk_parameter->ssi_inet_portnum);
	}
	if (stk_parameter->csi_hostport != -1 &&
	    stk_parameter->csi_hostport != 0) {
		fprintf(file_ptr, "csi_hostport = %d\n",
		    stk_parameter->csi_hostport);
		Trace(TR_OPRMSG, "csi_hostport = %d\n",
		    stk_parameter->csi_hostport);
	}
	fprintf(file_ptr, "capid = (acs=%d, lsm=%d, cap=%d)\n",
	    stk_parameter->stk_cap.acs_num, stk_parameter->stk_cap.lsm_num,
	    stk_parameter->stk_cap.cap_num);

	if (stk_parameter->stk_capacity_list->length > 0) {
	fprintf(file_ptr, "capacity = ( ");
	node_cap = stk_parameter->stk_capacity_list->head;
	ii = 1;
	while (node_cap != NULL) {
		stk_capacity = (stk_capacity_t *)node_cap->data;
		if (ii == 1) {
		fprintf(file_ptr, "%d = %llu ", stk_capacity->index,
		    stk_capacity->value);
		Trace(TR_OPRMSG, "%d = %llu ", stk_capacity->index,
		    stk_capacity->value);
		} else {
			fprintf(file_ptr, ", %d = %llu ", stk_capacity->index,
			    stk_capacity->value);
			Trace(TR_OPRMSG, ", %d = %llu ", stk_capacity->index,
			    stk_capacity->value);

		}
		node_cap = node_cap->next;
		ii ++;
	}
	fprintf(file_ptr, " )\n");
	}

	node_path = stk_parameter->stk_device_list->head;
	ii = 1;
	while (node_path != NULL) {
		stk_dev_path = (stk_device_t *)node_path->data;
		if (strcmp(stk_dev_path->pathname, drive_path) == 0) {
			stk_dev_path->shared = shared_state;
		}
		if (stk_dev_path->shared == B_TRUE) {
			fprintf(file_ptr, "%s = (acs=%d, lsm=%d, panel=%d,"
			    " drive=%d) shared\n",
			    stk_dev_path->pathname,
			    stk_dev_path->acs_num,
			    stk_dev_path->lsm_num,
			    stk_dev_path->panel_num,
			    stk_dev_path->drive_num);
		} else {
			Trace(TR_OPRMSG, "%s = (acs=%d, lsm=%d, panel=%d,"
			    " drive=%d)\n",
			    stk_dev_path->pathname,
			    stk_dev_path->acs_num,
			    stk_dev_path->lsm_num,
			    stk_dev_path->panel_num,
			    stk_dev_path->drive_num);
			fprintf(file_ptr, "%s = (acs=%d, lsm=%d, panel=%d,"
			    " drive=%d)\n",
			    stk_dev_path->pathname,
			    stk_dev_path->acs_num,
			    stk_dev_path->lsm_num,
			    stk_dev_path->panel_num,
			    stk_dev_path->drive_num);
		}
		node_path = node_path->next;
		ii ++;
	}
	fclose(file_ptr);

	Trace(TR_OPRMSG, "finished writing stk parameter file");
	return (0);
}
