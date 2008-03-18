/*
 * scsiid.c - scsi id media changer, tape drives, etc.
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

#pragma ident "$Revision: 1.12 $"

/*
 * Includes, Declarations and Local Data
 */
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/scsi/impl/uscsi.h>
#include <sys/scsi/generic/sense.h>
#include <sys/scsi/generic/commands.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#define	MC_INQ_DEVTYPE	8		/* standard inquiry device type */
#define	DATAIN		true		/* SCSI read command */
#define	DATAOUT		false		/* SCSI write command */
#define	REQUIRED	true		/* display ioctl failures */
#define	OPTIONAL	false		/* don't display ioctl failures */

/*
 * boolean data type
 */
typedef enum bool { false, true } bool_t;

/*
 * device data structure
 */
typedef struct devdata
{
	char logdevpath [PATH_MAX];	/* logical device path */
	bool_t verbose;			/* display debug info */
	int fd;				/* robot or tape file descriptor. */
	int devtype;			/* standard inquiry device type */
	bool_t encode;			/* encode space ch with %%20 */
	char vendor[9];			/* std inquiry vendor string */
	char product[17];		/* std inquiry product string */
} devdata_t;

/*
 * Local Function Prototypes
 */
int parse_cmd_line_args(int, char **, devdata_t *);
int issue_cmd(devdata_t *, char *, int, char *, int *, bool_t, bool_t);
int query_mc_dt(devdata_t *);
int usn_inquiry(devdata_t *);
int standard_inquiry(devdata_t *devdata);
int parse_identifier_descriptor(devdata_t *, char *, int, int *);
int devid_inquiry(devdata_t *);

/*
 * Defines
 */
#define	DPNC(ch) (isprint(ch) ? ch : ' ')
	/* don't print null chars; the ksh read command can't read nulls */
#define	ENCODE encode
	/* encode ' ' as %%20 and '%' as %%25 */


/*
 *	----- encode - encode space characters with %%20 and percent with %%25.
 *	Given a character, if its a space character encode as %%20, else if
 *	its a percent character encode as %%25.
 *	Returns output character string or  '%' and ' ' encoded string.
 */
char *
encode(
	bool_t enable,	/* string encode enabled. */
	char ch)	/* input ascii character. */
{
	static char buf [50];

	if (enable == true && ch == ' ')
		sprintf(buf, "%%%%20");
	else if (enable == true && ch == '%')
		sprintf(buf, "%%%%25");
	else
		sprintf(buf, "%c", ch);
	return (buf);
}

/*
 *	----- parse_cmd_line_args - parse command line arguments.
 *	Given command line arguments, store parsed results into device data
 *	structure.
 */
int				/* 0 is success. */
parse_cmd_line_args(
	int argc,		/* C program argument count. */
	char **argv,		/* argument strings. */
	devdata_t *devdata)	/* device data. */
{
	int opt;

	memset(devdata, 0, sizeof (devdata_t));

	while ((opt = getopt(argc, argv, "p:ev")) != EOF) {
		switch (opt) {
		case 'p':
			if (strlen(optarg) >= PATH_MAX) {
				printf("Invalid logical device path length.\n");
				return (1);
			}
			strncpy(devdata->logdevpath, optarg, PATH_MAX);
			break;

		case 'e':
			devdata->encode = true;
			break;

		case 'v':
			devdata->verbose = true;
			break;

		case '?':
			return (2);
		}
	}

	if (devdata->logdevpath [0] == '\0') {
		printf("usage: -p /dev/... [-v <verbose> ] [-e <encode>]\n");
		return (2);
	}

	return (0);
}

/*
 *	----- issue_cmd - uscsi command interface.
 *	Given device data, issue a SCSI Request Block(SRB) to a
 *	robot or tape drive.
 */
int				/* 0 is success. */
issue_cmd(
	devdata_t *devdata,	/* device data. */
	char *cdb,		/* command descriptor block (cdb). */
	int cdblen,		/* number of cdb bytes. */
	char *data,		/* datain or dataout buffer. */
	int *datalen,		/* buffer length. */
	bool_t datain,		/* data xfer direction. */
	bool_t required)	/* display errors. */
{
	int i;
	struct uscsi_cmd cmd;
	struct scsi_extended_sense sense;

	memset(&cmd, 0, sizeof (struct uscsi_cmd));

	cmd.uscsi_cdb = cdb;
	cmd.uscsi_cdblen = cdblen;

	memset(data, 0, *datalen);
	cmd.uscsi_bufaddr = data;
	cmd.uscsi_buflen = *datalen;

	cmd.uscsi_rqbuf = (char *)&sense;
	cmd.uscsi_rqlen = sizeof (struct scsi_extended_sense);

	cmd.uscsi_flags = USCSI_RQENABLE;
	cmd.uscsi_flags |= (datain == true ? USCSI_READ : USCSI_WRITE);
	if (devdata->verbose == false)
		cmd.uscsi_flags |= USCSI_SILENT;


	if (devdata->verbose == true) {
		printf("cdb: ");
		for (i = 0; i < cmd.uscsi_cdblen; i++)
			printf("%02x ", cmd.uscsi_cdb [i] & 0xff);
		printf("\n");
	}

	if (ioctl(devdata->fd, USCSICMD, &cmd) != 0) {
		if (required == true)
			printf("%s\n", strerror(errno));
		if (devdata->verbose) {
			printf("status: %02x key: %x asc: %02x ascq: %02x\n",
			    cmd.uscsi_status, sense.es_key,
			    sense.es_add_code, sense.es_qual_code);
		}
		return (1);
	}

	*datalen = cmd.uscsi_buflen - cmd.uscsi_resid;

	return (0);
}

/*
 *	----- query_mc_dt - Query media changer for data transport elements.
 *	Given device data, retrieve read element status data transport
 *	element descriptor data to display tape drive target, lun and
 *	sometimes serial number information.
 */
int				/* 0 is success. */
query_mc_dt(
	devdata_t *devdata)	/* device data. */
{
#define	LEN 0xffff
	char cdb [] = {(char)0xb8, 4, 0, 0, (char)0xff, (char)0xff, 0, 0,
		(char)(LEN>>8), (char)LEN, 0, 0};
	char data [LEN];
	int return_code, count, inc, i, j, datalen = LEN;
#define	DEVID 0x1
#define	DEVID_BYTE 6
	int len, elem, k;
	boolean_t l100 = B_FALSE;

	if ((memcmp(devdata->vendor, "M4 DATA", 7) == 0 &&
	    memcmp(devdata->product, "MagFile", 7) == 0) ||
	    (memcmp(devdata->vendor, "ATL", 3) == 0 &&
	    memcmp(devdata->product, "M2500", 5) == 0)) {
		cdb[1] |= 0x10;
		cdb[11] |= 0x80;
		l100 = B_TRUE;
	}

	/* Try and get SCSI-3 device id info 1st. */
	cdb [DEVID_BYTE] |= DEVID;
	if (issue_cmd(devdata, cdb, 12, data, &datalen, DATAIN, OPTIONAL)) {
		/* Try again getting using plain SCSI-2. */
		cdb [DEVID_BYTE] &= ~DEVID;
		if (return_code = issue_cmd(devdata, cdb, 12, data, &datalen,
		    DATAIN, REQUIRED))
			return (return_code);
	}

	printf("Read Element Status Data Transport Descriptor\n");
	printf("---------------------------------------------\n");

	count = (data [2] << 8) | data [3];
	printf("tape-drive-count: %d\n\n", count);

	inc = (data [10] << 8) | data [11];

	for (i = 0, j = 16; i < count; i++, j += inc) {
		/*
		 * Common SCSI-2 and SCSI-3 processing: element address,
		 * target and lun.
		 */
		elem = (data [j] << 8) | (unsigned char)data [j+1];
		printf("element-address: %x\n", elem);

		printf("not-bus: %d\n", (data [j+6] >> 7) & 0x1);

		printf("target: ");
		if (data [j+6] & 0x20)
			printf("%x", data [j+7]);
		printf("\n");

		printf("lun: ");
		if (data [j+6] & 0x10)
			printf("%x", data [j+6] & 0x7);
		printf("\n");

		if (l100 == B_TRUE) {
			bool_t found = false;
			for (k = j+52; ; k++) {
				if (data[k] == '\0')
					break;
				if (data[k] != ' ') {
					found = true;
					break;
				}
			}
			if (found == true) {
				printf("id: %s\n", &data[j+52]);
			} else {
				printf("id: -\n");
			}
		} else if (cdb [DEVID_BYTE] & DEVID) {

		/*
		 * SMC-2 specification indicates one identifier
		 * descriptor.
		 */
		if (parse_identifier_descriptor(devdata, &data [j+12],
		    datalen - j+12, &len))
			return (1);
		}

		printf("\n");
	}

	printf("\n");

	/* Number of storage elements. */
	cdb [1] = (cdb [1] & 0xf0) | 2;
	cdb [DEVID_BYTE] &= ~DEVID;
	if (issue_cmd(devdata, cdb, 12, data, &datalen, DATAIN, OPTIONAL)) {
		return (return_code);
	}

	printf("Read Element Status Storage Descriptor\n");
	printf("--------------------------------------\n");

	count = (data [2] << 8) | data [3];
	printf("storage-element-count: %d\n\n", count);

	return (0);
}

/*
 *	----- usn_inquiry - Unit serial number inquiry page 80.
 *	Given a robot or tape drive device data, retrieve
 *	and display unit serial number data.
 */
int				/* 0 is success. */
usn_inquiry(
	devdata_t *devdata)	/* device data. */
{
#define	USN_INQ_LEN 0xff
	char cdb [] = {0x12, 1, (char)0x80, 0, (char)USN_INQ_LEN, 0};
	char data [USN_INQ_LEN];
	int return_code, i, length = 0, datalen = USN_INQ_LEN;

	/*
	 * scsi standard says usn page is required, however since some
	 * devices like optical don't have usn pages make it optional.
	 */
	if ((return_code = issue_cmd(devdata, cdb, 6, data, &datalen,
	    DATAIN, OPTIONAL)) != 0)
		return (return_code);

	if (datalen >= 4)
		length = data [3];

	printf("Unit Serial Number Inquiry\n");
	printf("--------------------------\n");

	printf("unit-serial-number: ");
	for (i = 0; i < length; i++) {
		printf("%s", ENCODE(devdata->encode, DPNC(data[i+4])));
	}
	printf("\n");

	printf("\n");
	return (0);
}

/*
 *	----- standard_inquiry - Issue a standard SCSI inquiry command.
 *	Given a robot or tape drive device data, retrieve
 *	and display standard inquiry data.
 */
int				/* 0 is success. */
standard_inquiry(
	devdata_t *devdata)	/* device data. */
{
#define	STD_INQ_LEN 0xff
	char cdb [] = {0x12, 0, 0, 0, (char)STD_INQ_LEN, 0};
	char data [STD_INQ_LEN];
	int return_code, i, length = 0, datalen = STD_INQ_LEN;

	if ((return_code = issue_cmd(devdata, cdb, 6, data, &datalen,
	    DATAIN, REQUIRED)) != 0)
		return (return_code);

	memcpy(devdata->vendor, &data[8], 8);
	memcpy(devdata->product, &data[16], 16);

	printf("Standard Inquiry\n");
	printf("----------------\n");

	if (datalen >= 1)
		devdata->devtype = data [0] & 0x1f;
	printf("device-type: %x\n", devdata->devtype);

	printf("version: %02x\n", data [2] & 0xff);

	length = (datalen >= 16 ? 8 : 0);

	printf("vendor: ");
	for (i = 0; i < length; i++) {
		printf("%s", ENCODE(devdata->encode, DPNC(data[i+8])));
	}
	printf("\n");

	length = (datalen >= 32 ? 16 : 0);
	printf("product: ");
	for (i = 0; i < length; i++) {
		printf("%s", ENCODE(devdata->encode, DPNC(data[i+16])));
	}
	printf("\n");

	length = (datalen >= 35 ? 4 : 0);
	printf("revision: ");
	for (i = 0; i < length; i++) {
		printf("%s", ENCODE(devdata->encode, DPNC(data[i+32])));
	}
	printf("\n");

	printf("multiport: %d\n", (data[6] >> 4) & 1);

	printf("\n");
	return (0);
}

/*
 *	----- parse_identifier_descriptor - Parse identification descriptor.
 *	Given inquiry or read element status identification descriptor,
 *	display vital information.
 */
int				/* 0 is success. */
parse_identifier_descriptor(
	devdata_t *devdata,	/* device data. */
	char *desc,		/* binary description data. */
	int desclen,		/* binary description data length. */
	int *usedlen)		/* bytes processed. */
{
	int length = 0, i;
	int code_set, id_type;
	const int binary_values = 1,
	    ascii_graphic_codes = 2;

	if (desclen < 3)
		return (1);

	code_set = desc [0] & 0xf;
	printf("code-set: %x\n", code_set);

	printf("association: %x\n", (desc [2] >> 4) & 0x3);

	id_type = desc [1] & 0xf;
	printf("id-type: %x\n", id_type);

	if (desclen >= 4)
		length = desc [3];

	printf("id: ");
	for (i = 0; i < length && i < desclen; i++) {
		if (code_set == binary_values)
			printf("%02x", (unsigned char)desc[i+4]);
		else if (code_set == ascii_graphic_codes)
			printf("%s", ENCODE(devdata->encode, DPNC(desc[i+4])));
	}
	printf("\n");

	*usedlen = (i > 0 ? i + 4 : 0);

	return (0);
}

/*
 *	----- devid_inquiry - Device identification inquiry.
 *	Given a robot or tape drive device data, retrieve
 *	and display device identification data.
 */
int				/* 0 for success. */
devid_inquiry(
	devdata_t *devdata)	/* device data. */
{
#define	DEVID_LEN 0xff
	char cdb [] = {0x12, 1, (char)0x83, 0, (char)DEVID_LEN, 0};
	char data [DEVID_LEN];
	int return_code, datalen = DEVID_LEN;
	int i, length = 0, usedlen = 0;

	if ((return_code = issue_cmd(devdata, cdb, 6, data, &datalen,
	    DATAIN, OPTIONAL)) != 0)
		return (return_code);

	printf("Device Identification Inquiry\n");
	printf("-----------------------------\n");

	if (datalen >= 3)
		length = data [3];

	for (i = 0; i < length; i += usedlen) {

		/*
		 * More than one identifier descriptor is allowed.
		 */
		if (parse_identifier_descriptor(devdata, &data [i+4],
		    length - i, &usedlen))
			return (1);
	}

	printf("\n");
	return (0);
}

/*
 *	----- main - Device identification inquiry.
 *	Given -p path command line arguments, parse serial numbers, devid,
 *	wwn, scsi target lun, and robot data transport info. Calling routine
 *	puts the tape drives in the robots.
 */
int			/* 0 is for success. */
main(
	int argc,	/* C program argument count. */
	char **argv)	/* argument strings. */
{
	int return_code;
	devdata_t devdata;

	/*
	 * Parse command line arguments.
	 */
	if ((return_code = parse_cmd_line_args(argc, argv, &devdata)) != 0)
		return (return_code);

	/*
	 * Open logical tape drive "/dev/rmt/cbn" or
	 * library /dev/samst/c* path.
	 */
	if ((devdata.fd = open(devdata.logdevpath, O_RDONLY| O_NDELAY)) == -1) {
		printf("%s\n", strerror(errno));
		return (1);
	}

	/*
	 * Get standard inquiry page.
	 */
	if ((return_code = standard_inquiry(&devdata)) != 0) {
		close(devdata.fd);
		return (return_code);
	}

	/*
	 * Get inquiry unit serial number page.
	 */
	if ((return_code = usn_inquiry(&devdata)) != 0) {
		close(devdata.fd);
		return (return_code);
	}

	/*
	 * Get inquiry device identification page.
	 */
	(void) devid_inquiry(&devdata);

	/*
	 * If SCSI-3 media changer device type, get robot installed
	 * tape drive unit serial number from read element status
	 * device identification data.
	 *
	 * If SCSI-2 media changer device type, get robot installed
	 * tape drive parallel SCSI target and lun.
	 */
	if (devdata.devtype == MC_INQ_DEVTYPE) {
		return_code = query_mc_dt(&devdata);
	}

	close(devdata.fd);

	/*
	 * Return 0 for success and non-zero for failure.
	 */
	return (return_code);
}
