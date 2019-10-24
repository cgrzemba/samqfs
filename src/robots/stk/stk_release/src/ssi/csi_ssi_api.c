/**********************************************************************
*
*	C Source:		csi_ssi_api.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Fri Jan  6 09:19:29 1995 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: csi_ssi_api.c,1 %  (%full_filespec: 1,csrc,csi_ssi_api.c,1 %)";
#endif
/*
 * Copyright (1995, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_ssi_api_resp()
 *      
 * Description:
 *      Dispatcher of a network failure reponse back to client, if 
 *      there is a network failure in transmitting a request to the
 *      ACSLS server.
 *       
 *      o Determine version of request.
 *      o Dispatch to appropriate function for generating a 
 *        corresponding response.
 *      o Pass the generated response to csi_ipc_send(), which 
 *        will send the response to the correct client.
 *
 * Return Values: 
 *      status 
 * 
 * Implicit Inputs: 
 *      NONE 
 * 
 * Implicit Outputs: 
 *      NONE 
 * 
 * Considerations: 
 *      NONE 
 * 
 * Revision History: 
 *      Ken Stickney    5-Jan-1995      Original. 
 */ 
 
 
/* Header Files: */ 
#include "csi.h" 
#include "ml_pub.h" 

/* Procedure Type Declarations */

STATUS csi_v4_response( CSI_REQUEST *rqp, STATUS stat, int *size);
STATUS csi_v2_response( CSI_V2_REQUEST *rqp, STATUS stat, int *size);
STATUS csi_v1_response( CSI_V1_REQUEST *rqp, STATUS stat, int *size);
STATUS csi_v0_response( CSI_V0_REQUEST *rqp, STATUS stat, int *size);

void csi_ipc_send(CSI_MSGBUF *ntbfp);


STATUS csi_ssi_api_resp(CSI_MSGBUF *netbufp, STATUS response_status)
{
    int size = 0;
    STATUS status = STATUS_SUCCESS;

    VERSION version;

    CSI_REQUEST *reqp;

    CSI_V2_REQUEST *reqp_v2;
    CSI_V1_REQUEST *reqp_v1;
    CSI_V0_REQUEST *reqp_v0;



    reqp      = (CSI_REQUEST *) CSI_PAK_NETDATAP(netbufp);
    reqp_v2   = (CSI_V2_REQUEST *) CSI_PAK_NETDATAP(netbufp);
    reqp_v1   = (CSI_V1_REQUEST *) CSI_PAK_NETDATAP(netbufp);
    reqp_v0   = (CSI_V0_REQUEST *) CSI_PAK_NETDATAP(netbufp);

    /* determine version of request */
    if (reqp->csi_req_header.message_header.message_options & EXTENDED) {

        /* size version1 and later */
	version = reqp->csi_req_header.message_header.version;

        switch (version) {
            case VERSION4:
		status = csi_v4_response(reqp, response_status, &size);
		break;
            case VERSION3:
            case VERSION2:
		status = csi_v2_response(reqp_v2, response_status,
					 &size);
		break;
            case VERSION1:
		status = csi_v1_response(reqp_v1, response_status,
					 &size);
		break;
	    default:
		break;
	}

    } 
    else { /* this is a version 0 request packet */
 
		status = csi_v0_response(reqp_v0, response_status,
				       &size);
    }

    if( status == STATUS_SUCCESS ){

        netbufp->size = sizeof(CSI_HEADER) + size;

    /* send the appropriate response */
        csi_ipc_send(netbufp);
    }

    return status;
}
