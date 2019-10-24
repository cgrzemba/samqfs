static char CCM_Id[]="@(#) %full_name: 1/csrc/acs_mount_pinfo.c/andysb % %release: CSC400 % %date_modified: Wed Oct 20 09:19:24 1999 % (c) 1998 StorageTek";
#include "acssys.h"
/*
 *                        (c) Copyright (1999-2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_mount_pinfo()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      mount request to the ACSSS software.  A VIRTUAL_MOUNT request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE or
 *                   STATUS_INVALID_VALUE, or STATUS_PROCESS_FAILURE
 *
 * Parameters:
 *
 *  seqNumber     - A client defined number returned in the response.
 *  lockId        - Lock the drive with this id or NO_LOCK. 
 *  volId         - Id of the tape cartridge to be mounted.
 *  poolid        - Pool Id
 *  mgmt_cl       - Character array containing Management Class.
 *  media_type    - A numerical value corresponding to a tape cartridge
 *                  media type, or ANY_MEDIA_TYPE, or ALL_MEDIA_TYPE.
 *  scratch       - If TRUE, implies scratch request.
 *  readonly      - If TRUE, the volume will be mounted readonly.
 *  bypass        - If TRUE, bypass volser and media verification.
 *  jobname       - Character array containing Job Name.
 *  dsnname       - Character array containing Data Set Name.
 *  stepname      - Character array containing Step Name.
 *  driveId       - Id of the drive where the tape cartridge is mounted.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *    A. Perko             20-Oct-1999    Converted vir_mount to 
 *                                        mount_pinfo and removed
 *                                        DUPLEX parameter.
 */
 
#include <stddef.h>
#include <stdio.h>
#include <string.h>
 
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
 
#undef SELF
#define SELF "acs_mount_pinfo"
#undef ACSMOD
#define ACSMOD 52
 
STATUS acs_mount_pinfo
(
    SEQ_NO seqNumber,
    LOCKID lockId,
    VOLID volid,
    POOLID poolid,
    MGMT_CLAS mgmt_cl,
    MEDIA_TYPE media_type,
    BOOLEAN scratch, 
    BOOLEAN readonly,
    BOOLEAN bypass,
    JOB_NAME jobname,
    DATASET_NAME dsnname,
    STEP_NAME stepname,
    DRIVEID driveid
) 
{
    COPYRIGHT;
    STATUS acsReturn;
    MOUNT_PINFO_REQUEST mountpinfoRequest;
 
    acs_trace_entry ();
 
    acsReturn = acs_verify_ssi_running ();
 
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header ((char *) &mountpinfoRequest,
	    sizeof (MOUNT_PINFO_REQUEST),
	    seqNumber,
	    COMMAND_MOUNT_PINFO,
	    EXTENDED,
	    VERSION_LAST - 1,
	    lockId);
 
	if (acsReturn == STATUS_SUCCESS) {
	    if (scratch) {
		mountpinfoRequest.request_header.message_header.message_options
		    |= SCRATCH;
	    }
	    if (readonly) {
		mountpinfoRequest.request_header.message_header.message_options
		    |= READONLY;
	    }
	    if (bypass) {
		mountpinfoRequest.request_header.message_header.message_options
		    |= BYPASS;
	    }
	    mountpinfoRequest.vol_id = volid;
	    mountpinfoRequest.pool_id = poolid;
	    mountpinfoRequest.mgmt_clas = mgmt_cl;
	    mountpinfoRequest.media_type = media_type;
	    mountpinfoRequest.job_name = jobname;
	    mountpinfoRequest.dataset_name = dsnname;
	    mountpinfoRequest.step_name = stepname;
	    mountpinfoRequest.drive_id = driveid;
	    acsReturn = acs_send_request (&mountpinfoRequest,
				  sizeof (MOUNT_PINFO_REQUEST));
	}
    }
    acs_trace_exit (acsReturn);
    return acsReturn;
}
 
