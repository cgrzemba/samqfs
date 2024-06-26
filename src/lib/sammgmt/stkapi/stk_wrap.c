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
#pragma	ident	"$Revision: 1.5 $"
static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "pub/devstat.h"
#include "pub/lib.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "aml/shm.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/robots.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "sam/lib.h"
#include "sam/devnm.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/nl_samfs.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/device.h"
#include "acssys.h"
#include "acsapi.h"

/*
 * ACSAPI to communicate with the ACSLS library
 *
 * The ACSAPI is the standard public client interface to the ACSLS library
 * ACSAPI procedures communicate via interprocess communication (IPC) with
 * the Storage Server Interface (SSI) process running on the same client
 * machine. Client applications make procedure calls to perform ACSLS library
 * functions. The ACSAPI procedures are asynchronous in nature. The client
 * sends request messages to the ACSLS library and the ACSLS library sends
 * responses to the client. The following is the procedure to perform any
 * function on the ACSLS library:
 *
 * 1) The client creates a request packet and contacts the SSI process.
 *
 * 2) The client receives a status value indicating the success/failure of the
 * request initiation. The ACSAPI also assigns a sequence number to match the
 * request. Since requests do not necessarily get responded to in order, this
 * sequence number is the only key to match a request to a particular response
 *
 * 3) The client calls acs_response() to obtain the response. Three response
 * types may be generated by the ACSLS software: acknowledge, intermediate,
 * and final. A final response is generated by the ACSLS when the requested
 * operation is complete or if the request ins invalid
 *
 * 4) Client applications may be required to call acs_response() several times
 * (for acknowledge and intermediate responses)  before receiving a final
 * response. A request is considered to be outstanding until a final response is
 * sent by the ACSLS
 */

#define	ACS_DEF_PORTNUM	50004
#define	ACS_STAT(t)	char *(*t)(STATUS)
#define	ACS_QP(t)	STATUS (*t)(SEQ_NO, POOL[], ushort_t)
#define	ACS_QS(t)	STATUS (*t)(SEQ_NO, POOL[], ushort_t)
#define	ACS_RSP(t)	STATUS (*t)(int, SEQ_NO *, REQ_ID *, \
	ACS_RESPONSE_TYPE *, void *)
#define	ACS_SA(t)	STATUS (*t)(char *)
#define	ACS_SS(t)	STATUS (*t)(SEQ_NO, LOCKID, POOL, \
	VOLRANGE[], BOOLEAN, ushort_t)

static int
_acs_get_response(void *dlhandle, int sequence, void **buffer);


/*
 * import a list of volumes to the STK ACSLS library
 */
int
acs_import_vsns(
int portnum,
char *fifo_file,
long lpool,
int vol_count,
vsn_t vsn) {
	char	env_acs_portnum[80];
	void	*dlhandle;
	SEQ_NO  sequence = 0;
	POOL	pools[MAX_ID];
	ACS_STAT(dl_acs_status);
	ACS_QP(dl_acs_query_pool);
	ACS_QS(dl_acs_query_scratch);
	ACS_SA(dl_acs_set_access);
	ACS_SS(dl_acs_set_scratch);

	void	*buffer;
	int ii;
	int err;
	VOLRANGE vol_ids[MAX_ID];
	int fifo_fd;
	sam_cmd_fifo_t    cmd_block;

	if ((buffer = (void *)mallocer(4096)) == NULL) {
		return (-1);
	}

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.magic	= CMD_FIFO_MAGIC;
	cmd_block.slot	= ROBOT_NO_SLOT;
	cmd_block.part	= 0;
	/*
	 * ACSAPI procedures communicate via interprocess communication (IPC)
	 * with the Storage Server Interface (SSI) process running on the same
	 * client. The environment variable ACSAPI_SSI_SOCKET is the IPC
	 * location of the SSI. The SSI, in turn, communicates with the ACSLS
	 * library via the network
	 *
	 */
	snprintf(env_acs_portnum, sizeof (env_acs_portnum),
		"ACSAPI_SSI_SOCKET=%d",
		(portnum == 0) ? ACS_DEF_PORTNUM : portnum);
	putenv(env_acs_portnum);

	/* load the libapi.so and the address of the acs functions */
	if ((dlhandle = dlopen("libapi.so", RTLD_NOW | RTLD_GLOBAL)) == NULL) {

		samerrno = SE_SHARED_OBJECT_LIB_CANNOT_BE_LOADED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"libapi.so", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	dl_acs_status = (ACS_STAT()) dlsym(dlhandle, "acs_status");
	if (dl_acs_status == NULL) {

		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_status", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	/* fn to retrieve all scratch pools */
	dl_acs_query_pool = (ACS_QP()) dlsym(dlhandle, "acs_query_pool");
	if (dl_acs_query_pool == NULL) {

		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_query_pool", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}
	/* fn to retrieve a list of scratch volumes for a scratch pool */
	dl_acs_query_scratch = (ACS_QS()) dlsym(dlhandle, "acs_query_scratch");
	if (dl_acs_query_scratch == NULL) {

		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_query_scratch", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	/* fn to set the access control userid for all subsequent requests */
	dl_acs_set_access = (ACS_SA()) dlsym(dlhandle, "acs_set_access");
	if (dl_acs_set_access == NULL) {

		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_response", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	if (vsn[0] != '\0') {	/* if access set */
		dl_acs_set_access(&vsn[0]);
	}
	/* fn to set/reset scratch attributes */
	dl_acs_set_scratch = (ACS_SS()) dlsym(dlhandle, "acs_set_scratch");
	if (dl_acs_set_scratch == NULL) {

		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_set_scratch", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	/* Query the ACSLS software to get the pools */
	pools[0] = lpool;
	if ((err = dl_acs_query_pool(sequence++, pools, 1))) {

		samerrno = SE_ACS_QUERY_POOL_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), "");
		strlcat(samerrmsg, dl_acs_status(err), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	if (_acs_get_response(dlhandle, sequence, &buffer) != 0) {

		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	ACS_QUERY_POL_RESPONSE *qp_resp = (ACS_QUERY_POL_RESPONSE *)buffer;
	if ((qp_resp->query_pol_status != STATUS_SUCCESS) ||
		(qp_resp->count != 1) ||
		((&qp_resp->pool_status[0])->pool_id.pool != lpool) ||
		((&qp_resp->pool_status[0])->volume_count < vol_count)) {

		samerrno = SE_ACS_QUERY_POOL_OPTION_INCORRECT;
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	/* Get the scratch pools */
	if ((err = dl_acs_query_scratch(sequence++, pools, 1))) {

		samerrno = SE_ACS_QUERY_SCRATCH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), "");
		strlcat(samerrmsg, dl_acs_status(err), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	if (_acs_get_response(dlhandle, sequence, &buffer) != 0) {

		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	ACS_QUERY_SCR_RESPONSE *qs_resp = (ACS_QUERY_SCR_RESPONSE *)buffer;
	if (qs_resp->count == 0) {	/* <= */
		samerrno = SE_ACS_SCRATCH_NO_VOLUME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
		samerrno = SE_UNABLE_TO_OPEN_COMMAND_FIFO;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	for (ii = 0; ii < (int)qs_resp->count && vol_count > 0; ii++) {
		QU_SCR_STATUS *qs_status = &qs_resp->scr_status[ii];
		if (qs_status->status != STATUS_VOLUME_HOME) {
			/* skip if volume is not in home */
			continue;
		}
		memset(&vol_ids[0].endvol.external_label[0], 0, sizeof (VOLID));
		memset(&vol_ids[0].startvol.external_label[0],
			0, sizeof (VOLID));
		strlcpy(&vol_ids[0].startvol.external_label[0],
			qs_status->vol_id.external_label, sizeof (VOLID));
		strlcpy(&vol_ids[0].endvol.external_label[0],
			qs_status->vol_id.external_label, sizeof (VOLID));

		if ((err = dl_acs_set_scratch(
			sequence++,
			NO_LOCK_ID,
			lpool,
			vol_ids,
			FALSE,
			1))) {

			close(fifo_fd);
			samerrno = SE_ACS_SET_SCRATCH_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(samerrno), "");
			strlcat(samerrmsg, dl_acs_status(err), MAX_MSG_LEN);
			Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
			return (-1);
		}

		if (_acs_get_response(dlhandle, sequence, &buffer) != 0) {

			Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
			return (-1);
		}
		ACS_SET_SCRATCH_RESPONSE *ss_resp =
			(ACS_SET_SCRATCH_RESPONSE *)buffer;
		if (ss_resp->set_scratch_status == STATUS_SUCCESS) {
			if (ss_resp->vol_status[0] == STATUS_SUCCESS) {
				strlcpy(cmd_block.vsn,
					ss_resp->vol_id[0].
					external_label,
					sizeof (vsn_t));
				cmd_block.cmd = CMD_FIFO_ADD_VSN;
				write(fifo_fd, &cmd_block,
					sizeof (sam_cmd_fifo_t));
				vol_count--;
			}
		} else {
			if (ss_resp->set_scratch_status !=
				STATUS_VOLUME_ACCESS_DENIED) {

				close(fifo_fd);

				samerrno = SE_ACS_SET_SCRATCH_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
					GetCustMsg(samerrno), "");
				strlcat(samerrmsg, dl_acs_status(err),
					MAX_MSG_LEN);
				Trace(TR_ERR, "acsls import failed: %s",
					samerrmsg);
				return (-1);
			}
		}
	}
	close(fifo_fd);
	return (0);
}

/* helper function to get response from acs */
static int
_acs_get_response(void *dlhandle, int sequence, void **buffer) {

	ACS_RSP(dl_acs_response);
	ACS_RESPONSE_TYPE  type;
	REQ_ID   resp_req;
	SEQ_NO   resp_seq;

	int st;

	dl_acs_response = (ACS_RSP()) dlsym(dlhandle, "acs_response");
	if (dl_acs_response == NULL) {
		// could not map to libapi.so dl_acs_response
		samerrno = SE_MAPPING_SYMBOL_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			"acs_response", "");
		strlcat(samerrmsg, dlerror(), MAX_MSG_LEN);
		Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
		return (-1);
	}

	do {
		st = dl_acs_response(-1, &resp_seq, &resp_req, &type, buffer);
		if (st != STATUS_SUCCESS) {
			samerrno = SE_ACS_RES_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
			return (-1);
		}
		if (resp_seq != sequence) {
			samerrno = SE_ACS_RES_WRONG_SEQ;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "acsls import failed: %s", samerrmsg);
			return (-1);
		}
	} while (type != RT_FINAL);
	return (0);
}
