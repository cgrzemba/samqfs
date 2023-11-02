/*
 *  scsi_trace_decode.c
 *
 *  Program to dump a scsi_trace file.  Can optionally display entries
 *  for just one device.
 *
 *  Example output:
 *   eq70 Issue 10:23:50   F:6   move medium
 *	  cdb: a5 00 00 00  00 07 01 f4  00 00 00 40
 *
 *   eq70 Reply 10:23:50   F:6   move medium
 *	  cdb: a5 00 00 00  00 07 01 f4  00 00 00 40
 *	sense: 70 00 05 00  00 00 00 0a  00 00 00 00  53 02 00 00  00 00 00 00
 *	  Sense key 05, ASC 53, ASCQ 02  medium removal prevented
 *
 *  eq70:	The equipment number involved in the command or completion.
 *
 *  Issue:	Means we've issued the scsi command.
 *
 *  Reply:	Means the command completed.  Completion status is shown.
 *
 *  10:23:50:  The time of day when the command was sent or received.
 *
 *  F:6	The file descriptor on which the command was issued.
 *		Not useful for non-SUN analysts.
 *
 *  cdb:	The cdb(command descriptor block): the scsi command issued
 *
 *  sense:	The sense data. If the command erred, then sense data is
 *		obtained and displayed.  If the command didn't err, then
 *		this field is all zero.
 *
 *  Sense key ...:  Decoded sense data, showing the sense key, additional
 *		sense code and additional sense code qualifier.  These
 *		values define what error occurred.
 *
 *  medium removal prevented:  Ascii representation of the ASC, ASCQ info.
 *
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

#pragma ident "$Revision: 1.16 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define	DEFAULT_SCSI_TRACE_FILENAME "/tmp/sam_scsi_trace"
#define	TRACE_BUFFER_ENTRIES 1000
struct cdb_trace {
	int    eq;			/* equipment number */
	int    what;			/* 0 - issue, 1 - response */
	time_t now;			/* unix time */
	int    fd;			/* the fd the ioctl was issued on */
	unsigned char   cdb[12];	/* the cdb */
	unsigned char   sense[20];	/* ret'd sense, only valid if what=1 */
	/* returned sense == 30303030... means no sense was available */
};


extern char *scsiAdditionalSenseToString(int, int);
extern char *scsiCommandToString(int);
extern int samcftime(char *s, const char *format, const time_t *clock);

int
main(int argc, char **argv)
{
	boolean_t readFromFile = B_TRUE;
	int eq = 0;
	int traceFd;
	char *traceFilename = DEFAULT_SCSI_TRACE_FILENAME;
	struct cdb_trace *traceBuffer;
	struct cdb_trace *traceEntry;
	int i;
	char tbuf[100];
	int opt;

	while ((opt = getopt(argc, argv, "e:f:")) != EOF) {
		switch (opt) {
		case 'e':
			eq = atoi(optarg);
			break;
		case 'f':
			traceFilename = optarg;
			break;
		case '?':
			(void) fprintf(stderr,
			    "USAGE: scsi_trace_decode [-f tracefile] "
			    "[-e eq]\n");
		}
	}

	if ((traceFd = open(traceFilename, O_RDONLY)) == -1) {
		fprintf(stderr, "Error opening file %s: %s\n", traceFilename,
		    strerror(errno));
		exit(1);
	}

	if ((traceBuffer =
	    (struct cdb_trace *)malloc(sizeof (struct cdb_trace) *
	    TRACE_BUFFER_ENTRIES)) == NULL) {
		fprintf(stderr, "Error allocating memory for trace buffer\n");
		exit(1);
	}

	while (readFromFile) {
		memset((void *)traceBuffer, 0,
		    sizeof (struct cdb_trace) * TRACE_BUFFER_ENTRIES);
		if (read(traceFd, traceBuffer,
		    sizeof (struct cdb_trace) * TRACE_BUFFER_ENTRIES) == -1) {
			fprintf(stderr, "Error reading file %s: %s\n",
				traceFilename,
					strerror(errno));
			exit(1);
		}

		for (i = 0; i < TRACE_BUFFER_ENTRIES; i++) {
			traceEntry = &traceBuffer[i];
			if (traceEntry->eq == 0) {
				readFromFile = B_FALSE;
				break;
			}
			if (eq != 0 && traceEntry->eq != eq) continue;
			samcftime(tbuf, "%T", &traceEntry->now);

			printf("\neq%d %s %s   F:%d   %s\n",
			    traceEntry->eq,
			    traceEntry->what ? "Reply" : "Issue",
			    tbuf, traceEntry->fd,
			    scsiCommandToString(traceEntry->cdb[0]));
			printf("       cdb: %02x %02x %02x %02x  "
			    "%02x %02x %02x %02x  "
			    "%02x %02x %02x %02x\n",
			    traceEntry->cdb[0], traceEntry->cdb[1],
			    traceEntry->cdb[2], traceEntry->cdb[3],
			    traceEntry->cdb[4], traceEntry->cdb[5],
			    traceEntry->cdb[6], traceEntry->cdb[7],
			    traceEntry->cdb[8], traceEntry->cdb[9],
			    traceEntry->cdb[10], traceEntry->cdb[11]);
			if (traceEntry->what) {
				if (strncmp((char *)&traceEntry->sense[0],
				    "00000000000000000000", 20) != 0) {
					printf("     sense: %02x %02x %02x "
					    "%02x "
					    " %02x %02x %02x %02x  "
					    "%02x %02x %02x %02x "
					    " %02x %02x %02x %02x  "
					    "%02x %02x %02x %02x\n",
					    traceEntry->sense[0],
					    traceEntry->sense[1],
					    traceEntry->sense[2],
					    traceEntry->sense[3],
					    traceEntry->sense[4],
					    traceEntry->sense[5],
					    traceEntry->sense[6],
					    traceEntry->sense[7],
					    traceEntry->sense[8],
					    traceEntry->sense[9],
					    traceEntry->sense[10],
					    traceEntry->sense[11],
					    traceEntry->sense[12],
					    traceEntry->sense[13],
					    traceEntry->sense[14],
					    traceEntry->sense[15],
					    traceEntry->sense[16],
					    traceEntry->sense[17],
					    traceEntry->sense[18],
					    traceEntry->sense[19]);
			if ((traceEntry->sense[0] & 0xfe) == 0x70) {

				printf(
				    "       Sense key %02x, ASC %02x, "
				    "ASCQ %02x  %s\n",
				    traceEntry->sense[2]&0xf,
				    traceEntry->sense[12],
				    traceEntry->sense[13],
				    scsiAdditionalSenseToString(
				    traceEntry->sense[12],
				    traceEntry->sense[13]));
			}
				}
			}
		}
	}
}

char *
scsiAdditionalSenseToString(int asc, int ascq)
{
	struct decodeEntry {
		int asc;
		int ascq;
		char *string;
	} decodeTable[] = {
	0x00, 0x00, "no additional sense information",
	0x00, 0x01, "filemark detected",
	0x00, 0x02, "end-of-partition/medium detected",
	0x00, 0x03, "setmark detected",
	0x00, 0x04, "beginning-of-partition/medium detected",
	0x00, 0x05, "end-of-data detected",
	0x00, 0x06, "I/O process terminated",
	0x00, 0x11, "audio play operation in progress",
	0x00, 0x12, "audio play operation paused",
	0x00, 0x13, "audio play operation successfully completed",
	0x00, 0x14, "audio play operation stopped due to error",
	0x00, 0x15, "no current audio status to return",
	0x01, 0x00, "no index/sector signal",
	0x02, 0x00, "no seek complete",
	0x03, 0x00, "peripheral device write fault",
	0x03, 0x01, "no write current",
	0x03, 0x02, "excessive write errors",
	0x04, 0x00, "logical unit not ready, cause not reportable",
	0x04, 0x01, "logical unit is in process of becoming ready",
	0x04, 0x02, "logical unit not ready, initializing command required",
	0x04, 0x03, "logical unit not ready, manual intervention required",
	0x04, 0x04, "logical unit not ready, format in progress",
	0x05, 0x00, "logical unit does not respond to selection",
	0x06, 0x00, "reference position found",
	0x07, 0x00, "multiple peripheral devices selected",
	0x08, 0x00, "logical unit communication failure",
	0x08, 0x01, "logical unit communication time-out",
	0x08, 0x02, "logical unit communication parity error",
	0x09, 0x00, "track following error",
	0x09, 0x01, "tracking servo failure",
	0x09, 0x02, "focus servo failure",
	0x09, 0x03, "spindle servo failure",
	0x0a, 0x00, "error log overflow",
	0x0c, 0x00, "write error",
	0x0c, 0x01, "write error recovered with auto reallocation",
	0x0c, 0x02, "write error - auto reallocation failed",
	0x10, 0x00, "ID CRC or ECC error",
	0x11, 0x00, "unrecovered read error",
	0x11, 0x01, "read retries exhausted",
	0x11, 0x02, "error too long to correct",
	0x11, 0x03, "multiple read errors",
	0x11, 0x04, "unrecovered read error - auto reallocate failed",
	0x11, 0x05, "L-EC uncorrectable error",
	0x11, 0x06, "CIRC unrecovered error",
	0x11, 0x07, "data resynchronization error",
	0x11, 0x08, "incomplete block read",
	0x11, 0x09, "no gap found",
	0x11, 0x0a, "miscorrected error",
	0x11, 0x0b, "unrecovered read error - recommend reassignment",
	0x11, 0x0c, "unrecovered read error - recommend rewrite the data",
	0x12, 0x00, "address mark not found for id field",
	0x13, 0x00, "address mark not found for data field",
	0x14, 0x00, "recorded entity not found",
	0x14, 0x01, "record not found",
	0x14, 0x02, "filemark or setmark not found",
	0x14, 0x03, "end-of-data not found",
	0x14, 0x04, "block sequence error",
	0x15, 0x00, "random positioning error",
	0x15, 0x01, "mechanical positioning error",
	0x15, 0x02, "positioning error detected by read of medium",
	0x16, 0x00, "data synchronization mark error",
	0x17, 0x00, "recovered data with no error correction applied",
	0x17, 0x01, "recovered data with retries",
	0x17, 0x02, "recovered data with positive head offset",
	0x17, 0x03, "recovered data with negative head offset",
	0x17, 0x04, "recovered data with retries and/or circ applied",
	0x17, 0x05, "recovered data using previous sector ID",
	0x17, 0x06, "recovered data without ECC - data auto-reallocated",
	0x17, 0x07, "recovered data without ECC - recommend reassignment",
	0x17, 0x08, "recovered data without ECC - recommend rewrite",
	0x18, 0x00, "recovered data with error correction applied",
	0x18, 0x02, "recovered data - data auto-reallocated",
	0x18, 0x03, "recovered data with CIRC",
	0x18, 0x04, "recovered data with LEC",
	0x18, 0x05, "recovered data - recommend reassignment",
	0x18, 0x06, "recovered data - recommend rewrite",
	0x19, 0x00, "defect list error",
	0x19, 0x01, "defect list not available",
	0x19, 0x02, "defect list error in primary list",
	0x19, 0x03, "defect list error in grown list",
	0x1a, 0x00, "parameter list length error",
	0x1b, 0x00, "synchronous data transfer error",
	0x1c, 0x00, "defect list not found",
	0x1c, 0x01, "primary defect list not found",
	0x1c, 0x02, "grown defect list not found",
	0x1d, 0x00, "miscompare during verify operation",
	0x1e, 0x00, "recovered ID with ECC",
	0x20, 0x00, "invalid command operation code",
	0x21, 0x00, "logical block address out of range",
	0x21, 0x01, "invalid element address",
	0x24, 0x00, "invalid field in CDB",
	0x25, 0x00, "logical unit not supported",
	0x26, 0x00, "invalid field in parameter list",
	0x26, 0x01, "parameter not supported",
	0x26, 0x02, "parameter value invalid",
	0x26, 0x03, "threshold parameters not supported",
	0x27, 0x00, "write protected",
	0x28, 0x00, "not ready to ready transition(medium may have changed)",
	0x28, 0x01, "import or export element accessed",
	0x29, 0x00, "power on, reset, or bus device reset occurred",
	0x2a, 0x00, "parameters changed",
	0x2a, 0x01, "mode parameters changed",
	0x2a, 0x02, "log parameters changed",
	0x2b, 0x00, "copy cannot execute since host cannot disconnect",
	0x2c, 0x00, "command sequence error",
	0x2c, 0x01, "too many windows specified",
	0x2c, 0x02, "invalid combination of windows specified",
	0x2d, 0x00, "overwrite error on update in place",
	0x2f, 0x00, "commands cleared by another initiator",
	0x30, 0x00, "incompatible medium installed",
	0x30, 0x01, "cannot read medium - unknown format",
	0x30, 0x02, "cannot read medium - incompatible format",
	0x30, 0x03, "cleaning cartridge installed",
	0x31, 0x00, "medium format corrupted",
	0x31, 0x01, "format command failed",
	0x32, 0x00, "no defect spare location available",
	0x32, 0x01, "defect list update failure",
	0x33, 0x00, "tape length error",
	0x36, 0x00, "ribbon, ink, or toner failure",
	0x37, 0x00, "rounded parameter",
	0x39, 0x00, "saving parameters not supported",
	0x3a, 0x00, "medium not present",
	0x3b, 0x00, "sequential positioning error",
	0x3b, 0x01, "tape position error at beginning-of-medium",
	0x3b, 0x02, "tape position error at end-of-medium",
	0x3b, 0x03, "tape or electronic vertical forms unit not ready",
	0x3b, 0x04, "slew failure",
	0x3b, 0x05, "paper jam",
	0x3b, 0x06, "failed to sense top-of-form",
	0x3b, 0x07, "failed to sense bottom-of-form",
	0x3b, 0x08, "reposition error",
	0x3b, 0x09, "read past end of medium",
	0x3b, 0x0a, "read past beginning of medium",
	0x3b, 0x0b, "position past end of medium",
	0x3b, 0x0c, "position past beginning of medium",
	0x3b, 0x0d, "medium destination element full",
	0x3b, 0x0e, "medium source element empty",
	0x3d, 0x00, "invalid bits in identify message",
	0x3e, 0x00, "logical unit has not self-configured yet",
	0x3f, 0x00, "target operating conditions have changed",
	0x3f, 0x01, "microcode has been changed",
	0x3f, 0x02, "changed operating definition",
	0x3f, 0x03, "inquiry data has changed",
	0x43, 0x00, "message error",
	0x44, 0x00, "internal target failure",
	0x45, 0x00, "select or reselect failure",
	0x46, 0x00, "unsuccessful soft reset",
	0x47, 0x00, "scsi parity error",
	0x48, 0x00, "initiator detected error message received",
	0x49, 0x00, "invalid message error",
	0x4a, 0x00, "command phase error",
	0x4b, 0x00, "data phase error",
	0x4c, 0x00, "logical unit failed self-configuration",
	0x4e, 0x00, "overlapped commands attempted",
	0x50, 0x00, "write append error",
	0x50, 0x01, "write append position error",
	0x50, 0x02, "position error related to timing",
	0x51, 0x00, "erase failure",
	0x52, 0x00, "cartridge fault",
	0x53, 0x00, "media load or eject failed",
	0x53, 0x01, "unload tape failure",
	0x53, 0x02, "medium removal prevented",
	0x54, 0x00, "scsi to host system interface failure",
	0x55, 0x00, "system resource failure",
	0x57, 0x00, "unable to recover table-of-contents",
	0x58, 0x00, "generation does not exist",
	0x59, 0x00, "updated block read",
	0x5a, 0x01, "operator medium removal request",
	0x5a, 0x02, "operator selected write protect",
	0x5a, 0x03, "operator selected write permit",
	0x5b, 0x00, "log exception",
	0x5b, 0x01, "threshold condition met",
	0x5b, 0x02, "log counter at maximum",
	0x5b, 0x03, "log list codes exhausted",
	0x5c, 0x00, "rpl status change",
	0x5c, 0x01, "spindles synchronized",
	0x5c, 0x02, "spindles not synchronized",
	0x60, 0x00, "lamp failure",
	0x61, 0x00, "video acquisition error",
	0x61, 0x01, "unable to acquire video",
	0x61, 0x02, "out of focus",
	0x62, 0x00, "scan head positioning error",
	0x63, 0x00, "end of user area encountered on this track",
	0x64, 0x00, "illegal mode for this track",
	-1, -1, ""
	};
	struct decodeEntry *entry;

	for (entry = decodeTable; entry->asc != -1; entry++) {
		if (asc < entry->asc) break;
		if (asc == entry->asc) {
			if (ascq == entry->ascq) {
				return (entry->string);
			}
		}
	}
	return ("reserved ASC/ASCQ value");
}



char *
scsiCommandToString(int cmdCode)
{
	struct decodeEntry {
		int cmdCode;
		char *string;
	} decodeTable[] = {
		0x00, "test unit ready",
		0x01, "rewind",
		0x01, "rezero unit",
		0x03, "request sense",
		0x04, "format",
		0x04, "format unit",
		0x05, "read block limits",
		0x07, "initialize element status / reassign blocks",
		0x08, "get message(06) / read(06) / receive",
		0x0a, "print / send message(06) / send(06) / write(06)",
		0x0b, "seek(06) / slew and print",
		0x0f, "read reverse",
		0x10, "synchronize buffer / write filemarks",
		0x11, "space",
		0x12, "inquiry",
		0x13, "verify(06)",
		0x14, "recover buffered data",
		0x15, "mode select(06)",
		0x16, "reserve / reserve unit",
		0x17, "release / release unit",
		0x18, "copy",
		0x19, "erase",
		0x1a, "mode sense(06)",
		0x1b, "load unload / scan / stop print / stop start unit",
		0x1c, "receive diagnostic results",
		0x1d, "send diagnostic",
		0x1e, "prevent allow medium removal",
		0x24, "set window",
		0x25, "get window / read capacity / read cd-rom capacity",
		0x28, "get message(10) / read(10)",
		0x29, "read generation",
		0x2a, "send message(10) / send(10) / write(10)",
		0x2b, "locate / position to element / seek(10)",
		0x2c, "erase(10)",
		0x2d, "read updated block",
		0x2e, "write and verify(10)",
		0x2f, "verify(10)",
		0x30, "search data high(10)",
		0x31, "object position / search data equal(10)",
		0x32, "search data low(10)",
		0x33, "set limits(10)",
		0x34, "get data buffer status / pre-fetch / read position",
		0x35, "synchronize cache",
		0x36, "lock unlock cache",
		0x37, "read defect data(10)",
		0x38, "medium scan",
		0x39, "compare",
		0x3a, "copy and verify",
		0x3b, "write buffer",
		0x3c, "read buffer",
		0x3d, "update block",
		0x3e, "read long",
		0x3f, "write long",
		0x40, "change definition",
		0x41, "write same",
		0x42, "read sub-channel",
		0x43, "read toc",
		0x44, "read header",
		0x45, "play audio(10)",
		0x47, "play audio msf",
		0x48, "play audio track index",
		0x49, "play track relative(10)",
		0x4b, "pause resume",
		0x4c, "log select",
		0x4d, "log sense",
		0x55, "mode select(10)",
		0x5a, "mode sense(10)",
		0xa5, "move medium",
		0xa5, "play audio(12)",
		0xa6, "exchange medium",
		0xa8, "get message(12) / read(12)",
		0xa9, "play track relative(12)",
		0xaa, "send message(12) / write(12)",
		0xac, "erase(12)",
		0xae, "write and verify(12)",
		0xaf, "verify(12)",
		0xb0, "search data high(12)",
		0xb1, "search data equal(12)",
		0xb2, "search data low(12)",
		0xb3, "set limits(12)",
		0xb5, "request volume element address",
		0xb6, "send volume tag",
		0xb7, "read defect data(12)",
		0xb8, "read element status",
		0xe7, "initialize element status with range",
		-1, ""
	};
	struct decodeEntry *entry;

	for (entry = decodeTable; entry->cmdCode != -1; entry++) {
		if (cmdCode < entry->cmdCode) break;
		if (cmdCode == entry->cmdCode) {
			return (entry->string);
		}
	}
	return ("reserved or vendor command code");
}
