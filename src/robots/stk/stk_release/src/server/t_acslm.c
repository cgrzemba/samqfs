#ifndef lint
static char SccsId[] = "@(#)t_acslm.c	2.2 2/12/02 (c) 1989-2002 StorageTek";
#endif
/*
 ** Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *      acslm.c - main()
 *
 * Description:
 *      Release 2 ACSLM test stub.
 *      Test stub used as an acslm driver for various storager server tests.
 *      This program can be compiled with various compiler definitions to
 *      achieve various results as follows:
 *
 *      Receive request packets on input socket and send out both an
 *      acknowledge response and other responses down the "return
 *      socket" or to the ACSSA depending on the stage of development
 *      of this program at the time (development of this tool is
 *      on-going.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      argv[1] - parent process id
 *      argv[2] - this program's input socket name
 *      argv[3] - module number that this will be running as
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Module Test Plan:
 *      NONE
 *
 * Revision History:
 *      Gary Mueller    10-Jan-1989.    Created.
 *      Joseph Wishner  27-Feb-1989.    Added #ifdefs for writing requests or
 *                                      responses to the to_REQOUT/to_RESOUT
 *                                      sockets.
 *      Joseph Wishner  06-Mar-1989.    Writes changed to write to return_socket
 *      Joseph Wishner  24-Mar-1989.    Rewrote entire code.
 *      Joseph Wishner  24-Mar-1989.    Bugfix.  Increase lbuf size 128->256.
 *      Joseph Wishner  30-Mar-1989.    Bugfix.  Lower Query Mount packet's
 *                                      size calculation from size of DRIVEID
 *                                      to size of the array of drive-ids.
 *      Joseph Wishner  30-Mar-1989.    Added -DRESOUTFILE conditional compile
 *                                      supporting writing responses to a file.
 *      Joseph Wishner  27-Oct-1989.    Removed the conditional compile code.
 *      Joseph Wishner  02-Apr-1990.    Add release 2 command responses.
 *      Jim Montgomery  24-Jul-1990     Add release 2 functionality.
 *      Joseph Wishner  28-Sep-1991.    Add release 3 functions as follows:
 *                                      1) expand the cap query
 *      Scott Siao      17-Oct-1992     Changed bzeros to memsets.
 *      Ken Stickney    18-Jan-1994     Removed unnecessary printf.
 *      Ken Stickney    06-May-1994     Now can accept down level packets and
 *                                      converts them up to the latest version
 *                                      that the server supports.
 *      Ken Stickney    06-Dec-1994     Fix for BR#49, and BR#50.
 *      Ken Stickney    23-Dec-1994     Flint based changes that fixed
 *                                      BUS error problem during Solaris
 *                                      port.
 *
 *      Chris Higgins   23-Oct_2001     Added tests for event notification.
 *      Chris Higgins   13-Nov_2001     Added tests for display.
 *      Scott Siao      12-Feb-2002     Added tests for mount_pinfo, 
 *                                      query_subpool_name,
 *                                      query_drive_group.
 *      Scott Siao      01-Mar-2002     Changed VOL_MANUALL_DELETED to
 *                                      VOL_DELETED.
 *      Wipro(Hemendra) 21-Jun-2004	Support for mount/ dismount events (for CIM)
 *					Added st_register_resource_event_mount_drv_act
 *					      st_register_resource_event_dismount_drv_act
 *					Modified st_check_registration_res
 *					      st_unregister_res
 *					      st_register_reg_event_intermed_res
 *					      st_register_reg_event_final_res
 *	Mitch Black	30-Nov-2004	Changed member of hand_id from panel 
 *					to panel_id.
 *      Mike Williams   02-Jun-2010     Changed the return type of main to be
 *                                      type int. Changed the expected return
 *                                      type of signal to be (void *)
 */

/*      Header Files: */
# include <stdlib.h>
# include <stdio.h>
# include <signal.h>
# include <string.h>
# include <sys/wait.h>
# include <sys/file.h>
# include "acssys.h"
# include "acsapi.h"
# include "acssys_pvt.h"
# include "acslm.h"
# include "cl_pub.h"
# include "cl_log.h"
# include "cl_ipc_pub.h"

/*      Defines, Typedefs and Structure Definitions: */
# define SELF "t_acslm"

/*      Global and Static Variable Declarations: */
# ifndef SOLARIS
struct sigvec vec;				/* Structure used by sigvec() */
# endif
char lbuf[256];
static char	return_sock[SOCKET_NAME_SIZE];	/* return socket */

/*      Procedure Type Declarations: */
static void	st_ack_res(REQUEST_TYPE *req_pkp, 
			   ACKNOWLEDGE_RESPONSE *ack_resp, 
			   int *sizep);
static void	st_audit_res(REQUEST_TYPE *req_pkp, 
			     AUDIT_RESPONSE *audit_resp, 
			     int *sizep);
static void	st_audit_ires(REQUEST_TYPE *req_pkp, 
			      EJECT_RESPONSE *audit_resp, 
			      int *sizep);
static void	st_cancel_res(REQUEST_TYPE *req_pkp, 
			      CANCEL_RESPONSE *cancel_resp, 
			      int *sizep);
static		STATUS st_chk_access(REQUEST_TYPE *req_pkp, 
				     RESPONSE_TYPE *res_pkp);
static void	st_clearlock_res(REQUEST_TYPE *req_pkp, 
				 CLEAR_LOCK_RESPONSE *clrlock_rsp, 
				 int *sizep);
static void	st_def_pool_res(REQUEST_TYPE *req_pkp, 
				DEFINE_POOL_RESPONSE *dp_rsp, 
				int *sizep);
static void	st_del_pool_res(REQUEST_TYPE *req_pkp, 
				DELETE_POOL_RESPONSE *dp_rsp, 
				int *sizep);
static void	st_dismount_res(REQUEST_TYPE *req_pkp, 
				DISMOUNT_RESPONSE *dismount_resp, 
				int *sizep);
static void	st_lock_res(REQUEST_TYPE *req_pkp, 
			    LOCK_RESPONSE *lock_rsp, 
			    int *sizep);
static void	st_mount_res(REQUEST_TYPE *req_pkp, 
			     MOUNT_RESPONSE *mount_resp, 
			     int *sizep);
static void	st_mtsc_res(REQUEST_TYPE *req_pkp, 
			    MOUNT_SCRATCH_RESPONSE *mtsc_resp, 
			    int *sizep);
static void	st_enter_res(REQUEST_TYPE *req_pkp, 
			     ENTER_RESPONSE *enter_resp, 
			     int *sizep);
static void	st_venter_res(REQUEST_TYPE *req_pkp, 
			      ENTER_RESPONSE *venter_resp, 
			      int *sizep);
static void	st_eject_res(REQUEST_TYPE *req_pkp, 
			     EJECT_RESPONSE *eject_resp, 
			     int *sizep);
static void	st_xeject_res(REQUEST_TYPE *req_pkp, 
			      EJECT_RESPONSE *eject_resp, 
			      int *sizep);
static void	st_idle_res(REQUEST_TYPE *req_pkp, 
			    IDLE_RESPONSE *idle_resp, 
			    int *sizep);
static void	st_query_res(REQUEST_TYPE *req_pkp, 
			     QUERY_RESPONSE *query_resp, 
			     int *sizep);
static void	st_querylock_res(REQUEST_TYPE *req_pkp, 
				 QUERY_LOCK_RESPONSE *qul_rsp, 
				 int *sizep);
static void	st_start_res(REQUEST_TYPE *req_pkp, 
			     START_RESPONSE *start_resp, 
			     int *sizep);
static void	st_setcap_res(REQUEST_TYPE *req_pkp, 
			      SET_CAP_RESPONSE *setcap_resp, 
			      int *sizep);
static void	st_setcl_res(REQUEST_TYPE *req_pkp, 
			     SET_CLEAN_RESPONSE *setcl_resp, 
			     int *sizep);
static void	st_setsc_res(REQUEST_TYPE *req_pkp, 
			     SET_SCRATCH_RESPONSE *setsc_resp, 
			     int *sizep);
static void	st_unlock_res(REQUEST_TYPE *req_pkp, 
			      UNLOCK_RESPONSE *unl_rsp, 
			      int *sizep);
static void	st_vary_res(REQUEST_TYPE *req_pkp, 
			    VARY_RESPONSE *vary_resp, 
			    int *sizep);
static void	st_register_reg_event_intermed_res(REQUEST_TYPE *req_pkp, 
					       REGISTER_RESPONSE *
					       register_resp, 
					       int *sizep);
static void	st_register_reg_event_final_res(REQUEST_TYPE *req_pkp, 
					       REGISTER_RESPONSE *
					       register_resp, 
					       int *sizep);
static void	st_register_resource_event_hli_res(REQUEST_TYPE *req_pkp, 
					       REGISTER_RESPONSE *
					       register_resp, 
					       int *sizep);
static void	st_register_resource_event_scsi_res(REQUEST_TYPE *req_pkp, 
					       REGISTER_RESPONSE *
					       register_resp, 
					       int *sizep);
static void	st_register_resource_event_fsc_res(REQUEST_TYPE *req_pkp, 
					       REGISTER_RESPONSE *
					       register_resp, 
					       int *sizep);
static void     st_register_resource_event_lsmser_res(REQUEST_TYPE *req_pkp,
							 REGISTER_RESPONSE *register_resp,
							 int *sizep);
static void     st_register_resource_event_lsm_res(REQUEST_TYPE *req_pkp, 
						   REGISTER_RESPONSE *register_resp,
						   int *sizep);
static void     st_register_resource_event_drv_res(REQUEST_TYPE *req_pkp, 
						   REGISTER_RESPONSE *register_resp,
						   int *sizep);
static void     st_register_resource_event_mount_drv_act(REQUEST_TYPE *req_pkp,
						   REGISTER_RESPONSE *register_resp,
						   int *sizep);
static void     st_register_resource_event_dismount_drv_act(REQUEST_TYPE *req_pkp,
						   REGISTER_RESPONSE *register_resp,
						   int *sizep);
static void	st_register_vol_event_res(REQUEST_TYPE *req_pkp, 
					  REGISTER_RESPONSE *
					  register_resp, 
					  int *sizep);
static void	st_unregister_res(REQUEST_TYPE *req_pkp, 
				  UNREGISTER_RESPONSE *unregister_resp, 
				  int *sizep);
static void	st_check_registration_res(REQUEST_TYPE *req_pkp, 
					  CHECK_REGISTRATION_RESPONSE *
					  check_registration_resp, 
					  int *sizep);
static void	st_display_res(REQUEST_TYPE *req_pkp, 
			       DISPLAY_RESPONSE *display_resp, 
			       int *sizep);
static void	st_mount_pinfo_res(REQUEST_TYPE *req_pkp, 
			       MOUNT_PINFO_RESPONSE *mount_pinfo_resp, 
			       int *sizep);
static void	st_query_subpool_name_res(REQUEST_TYPE *req_pkp, 
			       QU_SPN_RESPONSE *query_subpool_name_resp, 
			       int *sizep);
static void	st_query_drive_group_res(REQUEST_TYPE *req_pkp, 
			       QU_DRG_RESPONSE *query_drive_group_resp, 
			       int *sizep);

# ifdef DEBUG
unsigned long trace_module;			/* module trace define value */
unsigned long	trace_value;

/* trace flag value */
#   define TRACE_ACSLM		   0x00000400L
# endif /* DEBUG */

int main(argc, argv)
int argc;
char		*argv[];
{	
	int		i, size;		/* size of packet */
	STATUS		retcode;		/* return status */
	REQUEST_TYPE	req_pk;                 /* request  packet */
	RESPONSE_TYPE	res_pk;                 /* response packet */
	IPC_HEADER	*ipc_hdrp;

	/* ipc header ptr */

	if ((retcode = cl_proc_init(TYPE_LM, argc, argv)) != 
		       STATUS_SUCCESS)
	{
		cl_log_unexpected(SELF, "cl_proc_init", retcode);
		exit((int) STATUS_PROCESS_FAILURE);
	}
# ifdef DEBUG
	trace_module = TRACE_ACSLM;
	if TRACE(0)
		cl_trace(SELF, 3, (unsigned long) argv[1], (unsigned
							    long) argv[
			 2], 
			 (unsigned long) argv[3]);

# endif /* DEBUG */

	/* register for SIGTERM signal */
	if (signal(SIGTERM, cl_sig_hdlr) == SIG_ERR)
	{
		sprintf(lbuf, "t_acslm: Signal SIGTERM failed.\n");
		cl_log_event(lbuf);
		retcode = cl_ipc_destroy();
		exit((int) STATUS_PROCESS_FAILURE);
	}
	/* loop waiting for requests */
	while (TRUE)
	{
		memset((char *)& req_pk, 0, sizeof(req_pk));/* clear out read buffer */

		size = sizeof(req_pk);
		if ((retcode = cl_ipc_read((char *)& req_pk, &size)) != 
			       STATUS_SUCCESS)
		{
			sprintf(lbuf, "acslm:cl_ipc_read status = %s\n", 
				cl_status(retcode));
			cl_log_event(lbuf);
		}
		if (STATUS_CANCELLED == retcode)
		{
			continue;
		}
# ifdef DEBUG
		sprintf(lbuf, "acslm: number of bytes read %d\n", size);
		cl_log_event(lbuf);
# endif

		strcpy(return_sock, req_pk.generic_request.ipc_header.
		       return_socket);
		memset((char *)& res_pk, 0, sizeof(res_pk));
		/* check to see if t_cdriver has set the user id field of the 
                                         * access id field of the message header. It should be set to 
                                         * "t_cdriver" 
                                         */
		retcode = st_chk_access(&(req_pk), &(res_pk));
		if (retcode != STATUS_SUCCESS)
		{
			continue;
		}
		/* convert down level requests to current level of request */
		if ((retcode = lm_cvt_req(&req_pk, &size)) != 
			       STATUS_SUCCESS)
		{
			sprintf(lbuf, "acslm:lm_cvt_req status = %s\n", 
				cl_status(retcode));
			cl_log_event(lbuf);
		}
		if (retcode == STATUS_INVALID_TYPE)
		{
			res_pk.generic_response.request_header = req_pk.generic_request;
			res_pk.generic_response.request_header.ipc_header.module_type = 
			  TYPE_LM;
			ipc_hdrp = &(res_pk.generic_response.
				     request_header.ipc_header);
			strcpy(ipc_hdrp->return_socket, my_sock_name);
			res_pk.generic_response.response_status.status = 
			  STATUS_INVALID_TYPE;
			res_pk.generic_response.response_status.type = 
			  TYPE_NONE;
			size = sizeof(RESPONSE_HEADER);
		}
		else 
		{
			/* format and send an acknowledge response */
			st_ack_res(&req_pk, (ACKNOWLEDGE_RESPONSE *)& 
				   res_pk, &size);
			retcode = cl_ipc_write(return_sock, (void *)& 
					       res_pk, size);
			if (STATUS_SUCCESS != retcode)
			{
				sprintf(lbuf, 
					"acslm: cl_ipc_write status:%s\n", 
					cl_status(retcode));
				cl_log_event(lbuf);
			}
# ifdef DEBUG
			sprintf(lbuf, 
				"acslm: Wrote acknowledge response: %d bytes", 
				size);
			cl_log_event(lbuf);
# endif

			memset((char *)& res_pk, 0, sizeof(res_pk));
			size = 0;
			switch (req_pk.generic_request.message_header.
				command)
			{
			  case COMMAND_AUDIT:
				st_audit_ires(&req_pk, &res_pk.
					      eject_response, &size);
				retcode = cl_ipc_write(return_sock, (
					  void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf, 
						"cl_ipc_write status = %s\n", 
						cl_status(retcode));
					cl_log_event(lbuf);
				}
				size = 0;
				st_audit_res(&req_pk, &res_pk.
					     audit_response, &size);
				break;
			  case COMMAND_CANCEL:
				st_cancel_res(&req_pk, &res_pk.
					      cancel_response, &size);
				break;
			  case COMMAND_CLEAR_LOCK:
				st_clearlock_res(&req_pk, &res_pk.
						 clear_lock_response, &
						 size);
				break;
			  case COMMAND_DELETE_POOL:
				st_del_pool_res(&req_pk, &res_pk.
						delete_pool_response, &
						size);
				break;
			  case COMMAND_DEFINE_POOL:
				st_def_pool_res(&req_pk, &res_pk.
						define_pool_response, &
						size);
				break;
			  case COMMAND_DISMOUNT:
				st_dismount_res(&req_pk, &res_pk.
						dismount_response, &
						size);
				break;
			  case COMMAND_EJECT:
				if ((EXTENDED & req_pk.generic_request.
				     message_header.message_options) && 
				    (RANGE & req_pk.generic_request.
				     message_header.extended_options))
				{
					st_xeject_res(&req_pk, &res_pk.
						      eject_response, &
						      size);
				}
				else 
				{
					st_eject_res(&req_pk, &res_pk.
						     eject_response, &
						     size);
				}
				break;
			  case COMMAND_ENTER:
				if ((EXTENDED & 
				     req_pk.generic_request.
				     message_header.message_options) && 
				    (VIRTUAL & 
				     req_pk.generic_request.
				     message_header.extended_options))
				{
					st_venter_res(&req_pk, &res_pk.
						      enter_response, &
						      size);
				}
				else 
				{
					st_enter_res(&req_pk, &res_pk.
						     enter_response, &
						     size);
				}
				break;
			  case COMMAND_IDLE:
				st_idle_res(&req_pk, &res_pk.
					    idle_response, &size);
				break;
			  case COMMAND_LOCK:
				st_lock_res(&req_pk, &res_pk.
					    lock_response, &size);
				break;
			  case COMMAND_MOUNT_SCRATCH:
				st_mtsc_res(&req_pk, &res_pk.
					    mount_scratch_response, &
					    size);
				break;
			  case COMMAND_MOUNT:
				st_mount_res(&req_pk, &res_pk.
					     mount_response, &size);
				break;
			  case COMMAND_QUERY:
				st_query_res(&req_pk, &res_pk.
					     query_response, &size);
				break;
			  case COMMAND_QUERY_LOCK:
				st_querylock_res(&req_pk, &res_pk.
						 query_lock_response, &
						 size);
				break;
			  case COMMAND_SET_CAP:
				st_setcap_res(&req_pk, &res_pk.
					      set_cap_response, &size);
				break;
			  case COMMAND_SET_CLEAN:
				st_setcl_res(&req_pk, &res_pk.
					     set_clean_response, &size);
				break;
			  case COMMAND_SET_SCRATCH:
				st_setsc_res(&req_pk, &res_pk.
					     set_scratch_response, &
					     size);
				break;
			  case COMMAND_START:
				st_start_res(&req_pk, &res_pk.
					     start_response, &size);
				break;
			  case COMMAND_UNLOCK:
				st_unlock_res(&req_pk, &res_pk.
					      unlock_response, &size);
				break;
			  case COMMAND_VARY:
				st_vary_res(&req_pk, &res_pk.
					    vary_response, &size);
				break;
			  case COMMAND_REGISTER:
				st_register_reg_event_intermed_res(&req_pk, &res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				}
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_vol_event_res(&req_pk, &res_pk.register_response, &size); 
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				}
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;
		 
				st_register_resource_event_hli_res(&req_pk, &res_pk.register_response, &size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;
		 		
				st_register_resource_event_scsi_res(&req_pk, &res_pk.register_response, &size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_resource_event_fsc_res(&req_pk, &res_pk.register_response, &size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_resource_event_lsmser_res(&req_pk,&res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_resource_event_lsm_res(&req_pk,&res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_resource_event_drv_res(&req_pk,&res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;
				
				st_register_resource_event_mount_drv_act(&req_pk,&res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

				st_register_resource_event_dismount_drv_act(&req_pk,&res_pk.register_response,&size);
				retcode = cl_ipc_write(return_sock, (void *)& res_pk, size);
				if (STATUS_SUCCESS != retcode)
				{
					sprintf(lbuf,"acslm: cl_ipc_write status:%s\n",cl_status(retcode));
					cl_log_event(lbuf);
				} 
				memset((char *)& res_pk, 0, sizeof(res_pk));
				size = 0;

			     	st_register_reg_event_final_res(&req_pk, &res_pk.register_response, &size);
				break;
			  case COMMAND_UNREGISTER:
				st_unregister_res(&req_pk, &res_pk.unregister_response, &size);
				break;
			  case COMMAND_CHECK_REGISTRATION:
				st_check_registration_res(&req_pk, &res_pk.check_registration_response, &size);
				break;
			  case COMMAND_DISPLAY:
                                st_display_res(&req_pk, &res_pk.display_response, &size);
				break;
			  case COMMAND_MOUNT_PINFO:
				st_mount_pinfo_res(&req_pk, &res_pk.mount_pinfo_response, 
						     &size);
				break;
			  default:
				sprintf(lbuf, "Invalid command %s\n", 
					cl_command(req_pk.
						   generic_request.
						   message_header.
						   command));
				cl_log_event(lbuf);
				break;
			  }
# if 0	 /*   No reason to sleep  */
			if (TRUE == intermediate)
			{
				sleep(4);
			  }
			else 
			{
				sleep(2);
			  }
# endif
			/* convert to down level response packet version */

			if ((retcode = lm_cvt_resp(&res_pk, &size)) != 
				       STATUS_SUCCESS)
			{
				sprintf(lbuf, 
					"acslm:lm_cvt_resp status = %s\n", 
					cl_status(retcode));
				cl_log_event(lbuf);
			  } 
		}
		/* send final response */
		if ((retcode = cl_ipc_write(return_sock, (void *)& 
					    res_pk, size)) != 
			       STATUS_SUCCESS)
		{
			sprintf(lbuf, "cl_ipc_write status = %s\n", 
				cl_status(retcode));
			cl_log_event(lbuf);
		}
# ifdef DEBUG
		sprintf(lbuf, "acslm: Wrote final response: %d bytes", 
			size);
		cl_log_event(lbuf);
# endif
	}
}
void
st_ack_res(REQUEST_TYPE *req_pkp,		/* request packet */
	   ACKNOWLEDGE_RESPONSE *ack_resp,	/* acknowledge response */
	   /* packet */
	   int *sizep)				/* packet size returned */
{
	ack_resp->request_header = req_pkp->generic_request;
	ack_resp->request_header.ipc_header.module_type = TYPE_LM;
	ack_resp->message_status.status = STATUS_VALID;
	ack_resp->message_status.type = TYPE_NONE;
	ack_resp->message_id = 1;
	ack_resp->request_header.message_header.message_options |= ACKNOWLEDGE;
	*sizep = sizeof(ACKNOWLEDGE_RESPONSE);
}
/*
 * Name:        
 *      st_audit_ires()
 *
 * Description:
 *      Formats intermediate response for an audit command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_audit_ires(req_pkp, audit_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
EJECT_RESPONSE	*audit_resp;			/* audit response packet */
int		*sizep;                         /* packet size returned */
{
	EJECT_ENTER	*imedp;                 /* audit intermediate response */
	unsigned short	i;

	/* loop counter and index into vol status */

	imedp = (EJECT_ENTER *) audit_resp;
	imedp->request_header = req_pkp->generic_request;
	imedp->request_header.ipc_header.module_type = TYPE_LM;
	imedp->request_header.message_header.message_options |= INTERMEDIATE;
	strcpy(imedp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	imedp->message_status.status = STATUS_ACS_NOT_IN_LIBRARY;
	imedp->message_status.type = TYPE_ACS;
	imedp->message_status.identifier.acs_id = 
						  req_pkp->audit_request.identifier.acs_id[0];
	imedp->cap_id = req_pkp->audit_request.cap_id;
	imedp->count = req_pkp->audit_request.count;
	for (i = 0; i < imedp->count; i++)
	{
		strcpy(imedp->volume_status[i].vol_id.external_label, 
		       "SPE007");
		imedp->volume_status[i].status.status = STATUS_VOLUME_NOT_IN_LIBRARY;
		imedp->volume_status[i].status.type = TYPE_VOLUME;
		strcpy(imedp->volume_status[i].status.identifier.vol_id.
		       external_label, 
		       "SPE007");
	}
	*sizep = (char *)& imedp->volume_status[imedp->count] - (char *) imedp;
}
/*
 * Name:        
 *      st_audit_res()
 *
 * Description:
 *      Formats final response for an audit command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_audit_res(req_pkp, audit_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
AUDIT_RESPONSE	*audit_resp;			/* audit response packet */
int		*sizep;                         /* packet size returned */
{
	ACS		acs_id;                 /* identifier */
	LSMID		lsm_id;                 /* identifier */
	PANELID         panel_id;		/* identifier */
	SUBPANELID	subpanel_id;		/* identifier */
	AU_ACS_STATUS	*acs_statp;		/* identifier status */
	AU_LSM_STATUS	*lsm_statp;		/* identifier status */
	AU_PNL_STATUS	*pnl_statp;		/* identifier status */
	AU_SUB_STATUS	*subpnl_statp;		/* identifier status */
	unsigned short	i;

	/* loop counter */

	audit_resp->request_header = req_pkp->generic_request;
	audit_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(audit_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	audit_resp->message_status.status = STATUS_SUCCESS;
	audit_resp->message_status.type = TYPE_NONE;
	audit_resp->cap_id = req_pkp->audit_request.cap_id;
	audit_resp->type = req_pkp->audit_request.type;
	audit_resp->count = req_pkp->audit_request.count;
	/* loop through identifier status structures */
	for (i = 0; i < audit_resp->count; i++)
	{
		switch (audit_resp->type)
		{
		  case TYPE_ACS:
			acs_statp = &audit_resp->identifier_status.acs_status[i];
			acs_id = req_pkp->audit_request.identifier.acs_id[i];
			acs_statp->acs_id = acs_id;
			acs_statp->status.status = STATUS_VALID;
			acs_statp->status.type = TYPE_ACS;
			acs_statp->status.identifier.acs_id = acs_id;
			*sizep += sizeof(AU_ACS_STATUS);
			break;
		  case TYPE_LSM:
			lsm_statp = &audit_resp->identifier_status.lsm_status[i];
			lsm_id = req_pkp->audit_request.identifier.lsm_id[i];
			lsm_statp->lsm_id = lsm_id;
			lsm_statp->status.status = STATUS_VALID;
			lsm_statp->status.type = TYPE_LSM;
			lsm_statp->status.identifier.lsm_id = lsm_id;
			*sizep += sizeof(AU_LSM_STATUS);
			break;
		  case TYPE_PANEL:
			pnl_statp = &audit_resp->identifier_status.panel_status[i];
			panel_id = req_pkp->audit_request.identifier.panel_id[i];
			pnl_statp->panel_id = panel_id;
			pnl_statp->status.status = STATUS_VALID;
			pnl_statp->status.type = TYPE_PANEL;
			pnl_statp->status.identifier.panel_id = panel_id;
			*sizep += sizeof(AU_PNL_STATUS);
			break;
		  case TYPE_SUBPANEL:
			subpnl_statp = &audit_resp->identifier_status.subpanel_status[i];
			subpanel_id = req_pkp->audit_request.identifier.subpanel_id[i];
			subpnl_statp->subpanel_id = subpanel_id;
			subpnl_statp->status.status = STATUS_VALID;
			subpnl_statp->status.type = TYPE_SUBPANEL;
			subpnl_statp->status.identifier.subpanel_id = subpanel_id;
			*sizep += sizeof(AU_SUB_STATUS);
			break;
		  default:
			sprintf(lbuf, 
				"audit error, identifier type: %d", 
				audit_resp->type);
			cl_log_event(lbuf);
			break;
		  }
	}
	*sizep += (char *)& audit_resp->identifier_status
	- (char *)& audit_resp->request_header;
}
/*
 * Name:        
 *      st_cancel_res()
 *
 * Description:
 *      Formats final response for an cancel command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_cancel_res(req_pkp, cancel_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
CANCEL_RESPONSE *cancel_resp;			/* cancel response packet */
int		*sizep;

/* packet size returned */
{
	cancel_resp->request_header = req_pkp->generic_request;
	cancel_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(cancel_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	cancel_resp->message_status.status = STATUS_SUCCESS;
	cancel_resp->message_status.type = TYPE_NONE;
	cancel_resp->request = req_pkp->cancel_request.request;
	*sizep = sizeof(CANCEL_RESPONSE);
}
/*
 * Name:        
 *      st_chk_access()
 *
 * Description:
 *      Formats final response for an cancel command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */

static STATUS
st_chk_access(REQUEST_TYPE *req_pkp, RESPONSE_TYPE *res_pkp)
{	
	int		size = 0;		/* size of packet */
	char		*uid_ptr;
	MESSAGE_HEADER	*msg_hdr;
	IPC_HEADER	*ipc_hdrp;		/* ipc header ptr */
	STATUS		status;
	STATUS		retcode;

	msg_hdr = &(req_pkp->generic_request.message_header);
	uid_ptr = &(msg_hdr->access_id.user_id.user_label[0]);
	if (strcmp("t_cdriver", uid_ptr) != 0)
	{
		switch (msg_hdr->command)
		{
			/* These are the requests under access control */
		  case COMMAND_DISMOUNT:
			strncpy(res_pkp->dismount_response.vol_id.
				external_label, 
				req_pkp->dismount_request.vol_id.
				external_label, 
				EXTERNAL_LABEL_SIZE);
			res_pkp->dismount_response.drive_id = 
							      req_pkp->dismount_request.drive_id;
			status = STATUS_VOLUME_ACCESS_DENIED;
			size = sizeof(DISMOUNT_RESPONSE);
			break;
		  case COMMAND_CLEAR_LOCK:
			res_pkp->clear_lock_response.type = 
							    req_pkp->lock_request.type;
			res_pkp->clear_lock_response.count = 0;
			size = (char *)& res_pkp->clear_lock_response.identifier_status
			       - (char *)& res_pkp->clear_lock_response.request_header;
			switch (req_pkp->clear_lock_request.type)
			{
			  case TYPE_VOLUME:
				status = STATUS_VOLUME_ACCESS_DENIED;
				break;
			  default:
				status = STATUS_COMMAND_ACCESS_DENIED;
				break;
			  }
			break;
		  case COMMAND_LOCK:
			res_pkp->lock_response.type = req_pkp->lock_request.type;
			res_pkp->lock_response.count = 0;
			size = (char *)& res_pkp->lock_response.identifier_status
			       - (char *)& res_pkp->lock_response.request_header;
			switch (req_pkp->lock_request.type)
			{
			  case TYPE_VOLUME:
				status = STATUS_VOLUME_ACCESS_DENIED;
				break;
			  default:
				status = STATUS_COMMAND_ACCESS_DENIED;
				break;
			  }
			break;
		  case COMMAND_MOUNT:
			strncpy(res_pkp->dismount_response.vol_id.
				external_label, 
				req_pkp->dismount_request.vol_id.
				external_label, 
				EXTERNAL_LABEL_SIZE);
			res_pkp->dismount_response.drive_id = 
							      req_pkp->dismount_request.drive_id;
			status = STATUS_VOLUME_ACCESS_DENIED;
			size = sizeof(MOUNT_RESPONSE);
			break;
		  default:
			status = STATUS_SUCCESS;
			break;
		  }
		if (status != STATUS_SUCCESS)
		{
			res_pkp->generic_response.request_header = req_pkp->generic_request;
			res_pkp->generic_response.request_header.ipc_header.module_type = 
			  TYPE_LM;
			ipc_hdrp = &(res_pkp->generic_response.
				     request_header.ipc_header);
			strcpy(ipc_hdrp->return_socket, my_sock_name);
			res_pkp->generic_response.response_status.status = status;
			res_pkp->generic_response.response_status.type = 
			  TYPE_NONE;
			/* send final response */
			if ((retcode = cl_ipc_write(return_sock, (void *)
						    res_pkp, size)) != 
				       STATUS_SUCCESS)
			{
				sprintf(lbuf, 
					"cl_ipc_write status = %s\n", 
					cl_status(retcode));
				cl_log_event(lbuf);
			}
# ifdef DEBUG
			sprintf(lbuf, 
				"acslm: Wrote final response: %d bytes", 
				size);
			cl_log_event(lbuf);
# endif
			return (status);
		  }
		else 
		{
			/* current request not under access control */
			return (STATUS_SUCCESS);
		  }
	}
	else 
	{
		/* correct user id supplied */
		return (STATUS_SUCCESS);
	}
}
/*
 * Name:        
 *      st_dismount_res()
 *
 * Description:
 *      Formats final response for an dismount command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_dismount_res(req_pkp, dismount_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
DISMOUNT_RESPONSE *dismount_resp;		/* dismount response packet */
int		*sizep;

/* packet size returned */
{
	dismount_resp->request_header = req_pkp->generic_request;
	dismount_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(dismount_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	dismount_resp->message_status.status = STATUS_SUCCESS;
	dismount_resp->message_status.type = TYPE_NONE;
	strncpy(dismount_resp->vol_id.external_label, 
		req_pkp->dismount_request.vol_id.external_label, 
		EXTERNAL_LABEL_SIZE);
	dismount_resp->drive_id = req_pkp->dismount_request.drive_id;
	*sizep = sizeof(DISMOUNT_RESPONSE);
}
/*
 * Name:        
 *      st_eject_res()
 *
 * Description:
 *      Formats final response for an eject command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_eject_res(req_pkp, eject_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
EJECT_RESPONSE	*eject_resp;			/* eject response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter */

	eject_resp->request_header = req_pkp->generic_request;
	eject_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(eject_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	eject_resp->message_status.status = STATUS_SUCCESS;
	eject_resp->message_status.type = TYPE_NONE;
	eject_resp->cap_id = req_pkp->eject_request.cap_id;
	eject_resp->count = req_pkp->eject_request.count;
	for (i = 0; i < eject_resp->count; i++)
	{
		strncpy(eject_resp->volume_status[i].vol_id.
			external_label, 
			req_pkp->eject_request.vol_id[i].external_label, 
			EXTERNAL_LABEL_SIZE);
		eject_resp->volume_status[i].status.status = STATUS_SUCCESS;
		eject_resp->volume_status[i].status.type = TYPE_CAP;
		eject_resp->volume_status[i].status.identifier.cap_id = 
		  req_pkp->eject_request.cap_id;
	}
	*sizep = (char *)& eject_resp->volume_status[eject_resp->count]
		 - (char *)& eject_resp->request_header;
}
/*
 * Name:        
 *      st_enter_res()
 *
 * Description:
 *      Formats final response for an enter command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_enter_res(req_pkp, enter_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
ENTER_RESPONSE	*enter_resp;			/* enter response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter */

	enter_resp->request_header = req_pkp->generic_request;
	enter_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(enter_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	enter_resp->message_status.status = STATUS_SUCCESS;
	enter_resp->message_status.type = TYPE_NONE;
	enter_resp->cap_id = req_pkp->enter_request.cap_id;
	enter_resp->count = MAX_ID;
	for (i = 0; i < enter_resp->count; i++)
	{
		sprintf(enter_resp->volume_status[i].vol_id.
			external_label, "SPE0%02d", i);
		enter_resp->volume_status[i].status.status = STATUS_SUCCESS;
		enter_resp->volume_status[i].status.type = TYPE_CAP;
		enter_resp->volume_status[i].status.identifier.cap_id = 
		  req_pkp->enter_request.cap_id;
	}
	*sizep = (char *)& enter_resp->volume_status[enter_resp->count]
		 - (char *)& enter_resp->request_header;
}
/*
 * Name:        
 *      st_xeject_res()
 *
 * Description:
 *      Formats final response for an extended eject command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_xeject_res(req_pkp, eject_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
EJECT_RESPONSE	*eject_resp;			/* eject response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter */

	eject_resp->request_header = req_pkp->generic_request;
	eject_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(eject_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	eject_resp->message_status.status = STATUS_SUCCESS;
	eject_resp->message_status.type = TYPE_NONE;
	eject_resp->cap_id = req_pkp->ext_eject_request.cap_id;
	eject_resp->count = req_pkp->ext_eject_request.count;
	for (i = 0; i < eject_resp->count; i++)
	{
		eject_resp->volume_status[i].vol_id = 
						      req_pkp->ext_eject_request.vol_range[i].startvol;
		eject_resp->volume_status[i].status.status = STATUS_SUCCESS;
		eject_resp->volume_status[i].status.type = TYPE_CAP;
		eject_resp->volume_status[i].status.identifier.cap_id = 
		  req_pkp->ext_eject_request.cap_id;
	}
	*sizep = (char *)& eject_resp->volume_status[eject_resp->count]
		 - (char *) eject_resp;
}
/*
 * Name:        
 *      st_idle_res()
 *
 * Description:
 *      Formats final response for an idle command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_idle_res(req_pkp, idle_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
IDLE_RESPONSE	*idle_resp;			/* idle response packet */
int		*sizep;

/* packet size returned */
{
	idle_resp->request_header = req_pkp->generic_request;
	idle_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(idle_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	idle_resp->message_status.status = STATUS_SUCCESS;
	idle_resp->message_status.type = TYPE_NONE;
	*sizep = sizeof(IDLE_RESPONSE);
}
/*
 * Name:        
 *      st_mount_res()
 *
 * Description:
 *      Formats final response for an mount command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_mount_res(req_pkp, mount_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
MOUNT_RESPONSE	*mount_resp;			/* mount response packet */
int		*sizep;

/* packet size returned */
{
	mount_resp->request_header = req_pkp->generic_request;
	mount_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(mount_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	mount_resp->message_status.status = STATUS_SUCCESS;
	mount_resp->message_status.type = TYPE_NONE;
	strncpy(mount_resp->vol_id.external_label, 
		req_pkp->mount_request.vol_id.external_label, 
		EXTERNAL_LABEL_SIZE);
	mount_resp->drive_id = req_pkp->mount_request.drive_id[0];
	*sizep = sizeof(MOUNT_RESPONSE);
}
/*
 * Name:        
 *      st_mtsc_res()
 *
 * Description:
 *      Formats final response for an mount scratch command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_mtsc_res(req_pkp, mtsc_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
MOUNT_SCRATCH_RESPONSE *mtsc_resp;		/* mount scratch response packet */
int		*sizep;

/* packet size returned */
{
	mtsc_resp->request_header = req_pkp->generic_request;
	mtsc_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(mtsc_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	mtsc_resp->message_status.status = STATUS_SUCCESS;
	mtsc_resp->message_status.type = TYPE_NONE;
	mtsc_resp->pool_id = req_pkp->mount_scratch_request.pool_id;
	strcpy(mtsc_resp->vol_id.external_label, "PB9999");
	mtsc_resp->drive_id = req_pkp->mount_scratch_request.drive_id[0];
	*sizep = sizeof(MOUNT_SCRATCH_RESPONSE);
}
/*
 * Name:        
 *      st_query_res()
 *
 * Description:
 *      Formats final response for an query command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_query_res(req_pkp, query_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
QUERY_RESPONSE	*query_resp;			/* query response packet */
int		*sizep;                         /* packet size returned */
{
	QU_ACS_STATUS	*acs_statp;		/* status response */
	QU_LSM_STATUS	*lsm_statp;		/* status response */
	QU_CAP_STATUS	*cap_statp;		/* status response */
	QU_CLN_STATUS	*clean_statp;		/* status response */
	QU_DRV_STATUS	*drive_statp;		/* status response */
	QU_MMI_RESPONSE *mminfo_statp;		/* status response */
	QU_MSC_STATUS	*mtscr_statp;		/* status response */
	QU_MNT_STATUS	*mount_statp;		/* status response */
	QU_VOL_STATUS	*volume_statp;		/* status response */
	QU_POL_STATUS	*pool_statp;		/* status response */
	QU_PRT_STATUS	*port_statp;		/* status response */
	QU_REQ_STATUS	*request_statp;         /* status response */
	QU_SCR_STATUS	*scratch_statp;         /* scratch status response */
	QU_SUBPOOL_NAME_STATUS *subpool_name_statp; /*subpool name stat resp*/
	QU_DRG_RESPONSE *virt_drive_statp; /*virtual drive stat resp*/
	unsigned short	i, j, k, l;		/* loop counters */
	unsigned short  jlimit, klimit, llimit; /* loop limit counters */
	int             drive_group_count;
	STATUS		status;                 /* return status */
	char		tbuf[EXTERNAL_LABEL_SIZE + 1];
	char		sbuf[SUBPOOL_NAME_SIZE + 1];

	query_resp->request_header = req_pkp->generic_request;
	query_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(query_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	query_resp->message_status.status = STATUS_SUCCESS;
	query_resp->message_status.type = TYPE_NONE;
	query_resp->type = req_pkp->query_request.type;
	switch (query_resp->type)
	{
	  case TYPE_ACS:
		if (req_pkp->query_request.select_criteria.acs_criteria.
		    acs_count
		    == 0)
		{
			query_resp->status_response.acs_response.acs_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.acs_response.acs_count = 
			  req_pkp->query_request.select_criteria.acs_criteria.acs_count;
		}
		for (i = 0; i < query_resp->status_response.
			 acs_response.acs_count; i++)
		{
			acs_statp = &query_resp->status_response.acs_response.
				    acs_status[i];
			acs_statp->state = STATE_OFFLINE_PENDING;
			acs_statp->freecells = 100;
			acs_statp->status = STATUS_SUCCESS;
			if (req_pkp->query_request.select_criteria.
			    acs_criteria.
			    acs_count == 0)
			{
				acs_statp->acs_id = i;
			}
			else 
			{
				acs_statp->acs_id = req_pkp->query_request.select_criteria.
						    acs_criteria.acs_id[i];
			}
			for (j = k = 0; k < 5; k++, j++)
			{
				acs_statp->requests.requests[k][0] = j;
				acs_statp->requests.requests[k][1] = j + 1;
			}
		}
		*sizep = (char *)& query_resp->status_response.acs_response.
			 acs_status[query_resp->status_response.acs_response.
			 acs_count] - (char *) query_resp;
		break;
	  case TYPE_CAP:
		/* extended packet */
		if (req_pkp->query_request.select_criteria.cap_criteria.
		    cap_count == 0)
		{
			query_resp->status_response.cap_response.cap_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.cap_response.cap_count = 
			  req_pkp->query_request.select_criteria.cap_criteria.cap_count;
		}
		/* save response cap count */
		j = query_resp->status_response.cap_response.cap_count;
		/* loop through COUNT number of QU_CAP_RESPONSES */
		for (i = 0; i < j; i++)
		{
			cap_statp = 
				    & query_resp->status_response.cap_response.cap_status[i];
			/* fill in CAPID */
			if (req_pkp->
			    query_request.select_criteria.cap_criteria.
			    cap_count == 0)
			{
				cap_statp->cap_id.lsm_id.acs = (ACS)((
									       int)
					  i / (int)(MAX_ID / 2));
				cap_statp->cap_id.lsm_id.lsm = (LSM)((
									       int)
					  i % (int)(MAX_ID / 7));
				cap_statp->cap_id.cap = (CAP)((int) i % (
							int)(MAX_ID / 
							     21));
			}
			else 
			{
				cap_statp->cap_id = 
						    req_pkp->query_request.select_criteria.cap_criteria.cap_id[i];
			}
			cap_statp->status = STATUS_AUDIT_ACTIVITY;
			cap_statp->cap_priority = (CAP_PRIORITY)9;
			cap_statp->cap_size = (unsigned short)100;
			cap_statp->cap_state = STATE_ONLINE;
			cap_statp->cap_mode = CAP_MODE_AUTOMATIC;
		}
		req_pkp->query_request.select_criteria.cap_criteria.cap_count = j;
		*sizep = (char *)& query_resp->status_response.cap_response.
			 cap_status[j] - (char *) query_resp;
		break;
	  case TYPE_CLEAN:
		if (req_pkp->query_request.select_criteria.vol_criteria.
		    volume_count == 0)
		{
			query_resp->status_response.clean_volume_response.
			volume_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.clean_volume_response.
			volume_count = 
				       req_pkp->query_request.select_criteria.vol_criteria.
				       volume_count;
		}
		for (i = 0; i < query_resp->status_response.
			 clean_volume_response.volume_count; i++)
		{
			clean_statp = &query_resp->status_response.clean_volume_response.
				      clean_volume_status[i];
			if (req_pkp->query_request.select_criteria.
			    vol_criteria.
			    volume_count == 0)
			{
				sprintf(clean_statp->vol_id.
					external_label, "PB03%02d", i);
			}
			else 
			{
				clean_statp->vol_id = req_pkp->query_request.select_criteria.
						      vol_criteria.volume_id[i];
			}
			clean_statp->home_location.panel_id.lsm_id.acs = (
				  ACS) i;
			clean_statp->home_location.panel_id.lsm_id.lsm = (
				  LSM) i + 1;
			clean_statp->home_location.panel_id.panel = (
				  PANEL) i + 2;
			clean_statp->home_location.row = (ROW) i + 3;
			clean_statp->home_location.col = (COL) i + 4;
			clean_statp->max_use = (unsigned short)1000 + i;
			clean_statp->current_use = (unsigned short)9 + i;
			clean_statp->status = STATUS_VOLUME_HOME;
			clean_statp->media_type = ANY_MEDIA_TYPE;
		}
		req_pkp->query_request.select_criteria.vol_criteria.volume_count = 
		  query_resp->status_response.clean_volume_response.volume_count;
		*sizep = (char *)& query_resp->status_response.clean_volume_response.
			 clean_volume_status[query_resp->status_response.
			 clean_volume_response.volume_count] - (char *) query_resp;
		break;
	  case TYPE_DRIVE:
		if (req_pkp->query_request.select_criteria.
		    drive_criteria.
		    drive_count == 0)
		{
			query_resp->status_response.drive_response.drive_count = 
			  MAX_ID;
		}
		else 
		{
			query_resp->status_response.drive_response.drive_count = 
			  req_pkp->query_request.select_criteria.drive_criteria.
			  drive_count;
		}
		for (i = 0; i < query_resp->status_response.
			 drive_response.drive_count; i++)
		{
			drive_statp = &query_resp->status_response.drive_response.
				      drive_status[i];
			drive_statp->status = STATUS_DRIVE_IN_USE;
			if (req_pkp->query_request.select_criteria.
			    drive_criteria.
			    drive_count == 0)
			{
				drive_statp->drive_id.panel_id.lsm_id.acs = i;
				drive_statp->drive_id.panel_id.lsm_id.lsm = i + 1;
				drive_statp->drive_id.panel_id.panel = 10;
				drive_statp->drive_id.drive = 3;
			}
			else 
			{
				drive_statp->drive_id = req_pkp->query_request.select_criteria.
							drive_criteria.drive_id[i];
			}
			drive_statp->state = STATE_ONLINE;
			drive_statp->drive_type = i + 4;
			sprintf(drive_statp->vol_id.external_label, 
				"PB03%02d", i);
		}
		req_pkp->query_request.select_criteria.drive_criteria.drive_count = 
		  query_resp->status_response.drive_response.drive_count;
		*sizep = (char *)& query_resp->status_response.drive_response.
			 drive_status[query_resp->status_response.drive_response.
			 drive_count] - (char *) query_resp;
		break;
	  case TYPE_LSM:
		if (req_pkp->query_request.select_criteria.lsm_criteria.
		    lsm_count
		    == 0)
		{
			query_resp->status_response.lsm_response.lsm_count = 
			  MAX_ID;
		}
		else 
		{
			query_resp->status_response.lsm_response.lsm_count = 
			  req_pkp->query_request.select_criteria.lsm_criteria.lsm_count;
		}
		for (i = 0; i < query_resp->status_response.
			 lsm_response.lsm_count; i++)
		{
			lsm_statp = &query_resp->status_response.
				    lsm_response.lsm_status[i];
			if (req_pkp->query_request.select_criteria.
			    lsm_criteria.
			    lsm_count == 0)
			{
				lsm_statp->lsm_id.acs = i;
				lsm_statp->lsm_id.lsm = i + 1;
			}
			else 
			{
				lsm_statp->lsm_id = req_pkp->query_request.select_criteria.
						    lsm_criteria.lsm_id[i];
			}
			lsm_statp->state = STATE_OFFLINE_PENDING;
			lsm_statp->freecells = 100;
			lsm_statp->status = STATUS_CAP_AVAILABLE;
			for (j = k = 0; k < 5; k++, j++)
			{
				lsm_statp->requests.requests[k][0] = j;
				lsm_statp->requests.requests[k][1] = j + 1;
			}
		}
		req_pkp->query_request.select_criteria.lsm_criteria.lsm_count = 
		  query_resp->status_response.lsm_response.lsm_count;
		*sizep = (char *)& query_resp->status_response.lsm_response.
			 lsm_status[query_resp->status_response.
			 lsm_response.lsm_count] - (char *) query_resp;
		break;
	  case TYPE_MIXED_MEDIA_INFO:
		{
			QU_MMI_RESPONSE dummy[2];
			long		diff, diff2;

			mminfo_statp = &query_resp->
				       status_response.mm_info_response;
			*sizep = (char *)& query_resp->status_response
				 - (char *) query_resp;
			diff = (long)(&dummy[1]);
			diff2 = (long)(&dummy[0]);
			*sizep = *sizep + abs(diff - diff2);
			mminfo_statp->media_type_count = 2;
			mminfo_statp->media_type_status[0].media_type = 1;
			mminfo_statp->media_type_status[1].media_type = 4;
			strcpy(mminfo_statp->media_type_status[0].
			       media_type_name, 
			       "media_1");
			strcpy(mminfo_statp->media_type_status[1].
			       media_type_name, 
			       "media_4");
			mminfo_statp->media_type_status[0].cleaning_cartridge = 
			  CLN_CART_INDETERMINATE;
			mminfo_statp->media_type_status[1].cleaning_cartridge = 
			  CLN_CART_NEVER;
			mminfo_statp->media_type_status[0].max_cleaning_usage = 13;
			mminfo_statp->media_type_status[1].max_cleaning_usage = 29;
			mminfo_statp->media_type_status[0].compat_count = 1;
			mminfo_statp->media_type_status[1].compat_count = 2;
			mminfo_statp->media_type_status[0].compat_drive_types[0] = 2;
			mminfo_statp->media_type_status[1].compat_drive_types[0] = 2;
			mminfo_statp->media_type_status[1].compat_drive_types[1] = 3;
			mminfo_statp->drive_type_count = 2;
			mminfo_statp->drive_type_status[0].drive_type = 2;
			mminfo_statp->drive_type_status[1].drive_type = 3;
			strcpy(mminfo_statp->drive_type_status[0].
			       drive_type_name, 
			       "drive_2");
			strcpy(mminfo_statp->drive_type_status[1].
			       drive_type_name, 
			       "drive_3");
			mminfo_statp->drive_type_status[0].compat_count = 2;
			mminfo_statp->drive_type_status[1].compat_count = 1;
			mminfo_statp->drive_type_status[0].compat_media_types[0] = 1;
			mminfo_statp->drive_type_status[0].compat_media_types[1] = 4;
			mminfo_statp->drive_type_status[1].compat_media_types[0] = 4;
		}
		break;
	  case TYPE_MOUNT:
		{
			/* start in-line declaration block */

			unsigned short	original_count;

			/*
                                                         *      Break up into intermediate packets.  Use the "query_resp"
                                                         *      mount_status[0] as a buffer, becomes a count = 1 packet.
                                                         *      The last packet in the outer "for loop" below becomes the
                                                         *      final packet.
                                                         */
			/* save count of packet to use in for loop before we set it to 1 */
			if (req_pkp->query_request.select_criteria.
			    vol_criteria.
			    volume_count == 0)
			{
				query_resp->status_response.mount_response.
				mount_status_count = MAX_ID;
			}
			else 
			{
				query_resp->status_response.mount_response.
				mount_status_count = 
						     req_pkp->query_request.select_criteria.vol_criteria.
						     volume_count;
			}
			original_count = query_resp->status_response.mount_response.
					 mount_status_count;
			/* set to start, breaking into intermediate packets, count = 1 */
			query_resp->request_header.message_header.message_options |= 
			INTERMEDIATE;
			mount_statp = &query_resp->status_response.mount_response.
				      mount_status[0];
			query_resp->status_response.mount_response.mount_status_count = 1;
			*sizep = (char *)& query_resp->status_response.mount_response.
				 mount_status[query_resp->status_response.mount_response.
				 mount_status_count] - (char *) query_resp;
			/* build intermediate packets */
			for (i = 0; i < original_count; i++)
			{
				/* initialize the mount status array from request packet array */
				mount_statp->status = STATUS_VOLUME_HOME;
				strncpy(mount_statp->vol_id.
					external_label, 
					req_pkp->query_request.
					select_criteria.vol_criteria.
					volume_id[i].external_label, 
					EXTERNAL_LABEL_SIZE);
				/* drive count artificially set */
				mount_statp->drive_count = 10;
				/* initialize the drive status */
				for (j = 0; j < mount_statp->
					 drive_count; j++)
				{
					drive_statp = 
						      & query_resp->status_response.mount_response.
						      mount_status[i].drive_status[j];
					drive_statp->drive_id.panel_id.lsm_id.acs = j;
					drive_statp->drive_id.panel_id.lsm_id.lsm = j + 1;
					drive_statp->drive_id.panel_id.panel = j + 2;
					drive_statp->drive_id.drive = j + 3;
					drive_statp->drive_type = j + 4;
					sprintf(drive_statp->vol_id.
						external_label, 
						"PB04%02d", j);
					drive_statp->state = STATE_OFFLINE;
					drive_statp->status = STATUS_DRIVE_IN_USE;
				}
				/* done if only have one left, that one becomes the final reponse */
				if (i + 1 == original_count)
				{
					break;
				}
				else 
				{
					if ((status = cl_ipc_write(
						      return_sock, (
						      void *) query_resp, 
						      *sizep)) != 
						      STATUS_SUCCESS)
					{
						sprintf(lbuf, 
							"Error-Query-Mount cl_ipc_write(), status= %s", 
							cl_status(
							status));
						cl_log_event(lbuf);
					}
				}
			}
			/* set up the last drive status returned as the final response */
			query_resp->request_header.message_header.message_options = req_pkp->
			  generic_request.message_header.message_options;
		}
		/* end in-line declaration block */
		break;
	  case TYPE_MOUNT_SCRATCH_PINFO:
	  case TYPE_MOUNT_SCRATCH:
		{
			/* start in-line declaration block */

			unsigned short	original_count;
			QUERY_REQUEST	*q_req_ptr;
			QU_MSC_CRITERIA *qmsc_ptr;
			QU_MSC_RESPONSE *qmsr_ptr;

			q_req_ptr = &(req_pkp->query_request);
			/*
                                                        * get some pointers to the underlying data 
                                                        * structures of interest.
                                                        */

			qmsc_ptr = &(q_req_ptr->select_criteria.
				     mount_scratch_criteria);
			qmsr_ptr = &(query_resp->status_response.
				     mount_scratch_response);
			/*
                                                         *      Break up into intermediate packets.  Use the "query_resp"
                                                         *      mount_scratch_status[0] as a buffer, becomes a count = 1 packet.
                                                         *      The last packet in the outer "for loop" below becomes the
                                                         *      final packet.
                                                         */
			/* save count of packet to use in for loop before we set it to 1 */

			if (qmsc_ptr->pool_count == 0)
			{
				qmsr_ptr->msc_status_count = MAX_ID;
			}
			else 
			{
				qmsr_ptr->msc_status_count = qmsc_ptr->pool_count;
			}
			original_count = qmsr_ptr->msc_status_count;
			/* set to start, breaking into intermediate packets, count = 1 */
			query_resp->request_header.message_header.message_options |= 
			INTERMEDIATE;
			mtscr_statp = &qmsr_ptr->mount_scratch_status[0];
			qmsr_ptr->msc_status_count = 1;
			*sizep = (char *)& qmsr_ptr->
				 mount_scratch_status[qmsr_ptr->msc_status_count] - 
				 (char *) query_resp;
			/* build intermediate packets */
			for (i = 0; i < original_count; i++)
			{
				/* initialize the mount status array from request packet array */
				mtscr_statp->pool_id = qmsc_ptr->pool_id[i];
				mtscr_statp->status = STATUS_POOL_LOW_WATER;
				/* drive count artificially set */
				mtscr_statp->drive_count = 10;
				/* initialize the drive status */
				for (j = 0; j < mtscr_statp->
					 drive_count; j++)	  
				{
					drive_statp = &qmsr_ptr->mount_scratch_status[i].drive_list[j];
					drive_statp->drive_id.panel_id.lsm_id.acs = j;
					drive_statp->drive_id.panel_id.lsm_id.lsm = j + 1;
					drive_statp->drive_id.panel_id.panel = j + 2;
					drive_statp->drive_id.drive = j + 3;
					drive_statp->drive_type = j + 4;
					sprintf(drive_statp->vol_id.
						external_label, 
						"PB03%02d", j);
					drive_statp->state = STATE_ONLINE;
					drive_statp->status = STATUS_DRIVE_IN_USE;
				}
				/* done if only have one left, that one becomes the final reponse */
				if (i + 1 == original_count)
				{
					break;
				}
				else 
				{
					if ((status = cl_ipc_write(
						      return_sock, (
						      char *) query_resp, 
						      *sizep)) != 
						      STATUS_SUCCESS)
					{
						sprintf(lbuf, 
							"Error-Query-Mount cl_ipc_write(), status= %s", 
							cl_status(
							status));
						cl_log_event(lbuf);
					}
				}
			}
			/* set up the last drive status returned as the final response */
			query_resp->request_header.message_header.message_options = req_pkp->
			  generic_request.message_header.message_options;
		}
		/* end in-line declaration block */
		break;
	  case TYPE_POOL:
		if (req_pkp->query_request.select_criteria.
		    pool_criteria.pool_count
		    == 0)
		{
			query_resp->status_response.pool_response.
			pool_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.pool_response.pool_count = 
			  req_pkp->query_request.select_criteria.pool_criteria.pool_count;
		}
		for (i = 0; i < query_resp->status_response.
			 pool_response.pool_count; i++)
		{
			pool_statp = &query_resp->status_response.pool_response.
				     pool_status[i];
			if (req_pkp->query_request.select_criteria.
			    pool_criteria.
			    pool_count == 0)
			{
				pool_statp->pool_id.pool = i;
			}
			else 
			{
				pool_statp->pool_id = req_pkp->query_request.select_criteria.
						      pool_criteria.pool_id[i];
			}
			pool_statp->volume_count = (unsigned long)62 + i;
			pool_statp->low_water_mark = (unsigned long)20 + i;
			pool_statp->high_water_mark = (unsigned long)40 + i;
			pool_statp->pool_attributes = (unsigned long) OVERFLOW;
			pool_statp->status = STATUS_POOL_HIGH_WATER;
		}
		*sizep = (char *)& query_resp->status_response.pool_response.
			 pool_status[query_resp->status_response.pool_response.
			 pool_count] - (char *) query_resp;
		break;
	  case TYPE_PORT:
		if (req_pkp->query_request.select_criteria.
		    port_criteria.
		    port_count == 0)
		{
			query_resp->status_response.port_response.
			port_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.port_response.
			port_count = 
				     req_pkp->query_request.select_criteria.port_criteria.
				     port_count;
		}
		for (i = 0; i < query_resp->status_response.
			 port_response.port_count; i++)
		{
			port_statp = &query_resp->status_response.port_response.
				     port_status[i];
			if (req_pkp->query_request.select_criteria.
			    port_criteria.
			    port_count == 0)
			{
				port_statp->port_id.acs = i;
				port_statp->port_id.port = i + 1;
			}
			else 
			{
				port_statp->port_id = req_pkp->query_request.select_criteria.
						      port_criteria.port_id[i];
			}
			port_statp->state = STATE_OFFLINE_PENDING;
			port_statp->status = STATUS_SUCCESS;
		}
		*sizep = (char *)& query_resp->status_response.port_response.
			 port_status[query_resp->status_response.port_response.
			 port_count] - (char *) query_resp;
		break;
	  case TYPE_REQUEST:
		if (req_pkp->query_request.select_criteria.
		    request_criteria.
		    request_count == 0)
		{
			query_resp->status_response.request_response.
			request_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.request_response.
			request_count = 
					req_pkp->query_request.select_criteria.request_criteria.
					request_count;
		}
		for (i = 0; i < query_resp->status_response.
			 request_response.request_count; i++)
		{
			request_statp = &query_resp->status_response.request_response.
					request_status[i];
			if (req_pkp->query_request.select_criteria.
			    request_criteria.
			    request_count == 0)
			{
				request_statp->request = i;
			}
			else 
			{
				request_statp->request = req_pkp->query_request.
							 select_criteria.request_criteria.request_id[i];
			}
			request_statp->command = COMMAND_DISMOUNT;
			request_statp->status = STATUS_PENDING;
		}
		*sizep = (char *)& query_resp->status_response.request_response.
			 request_status[query_resp->status_response.request_response.
			 request_count] - (char *) query_resp;
		break;
	  case TYPE_SCRATCH:
		if (req_pkp->query_request.select_criteria.
		    pool_criteria.
		    pool_count == 0)
		{
			query_resp->status_response.scratch_response.
			volume_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.scratch_response.
			volume_count = 
				       req_pkp->query_request.select_criteria.pool_criteria.
				       pool_count;
		}
		for (i = 0; i < query_resp->status_response.
			 scratch_response.volume_count; i++)
		{
			scratch_statp = &query_resp->status_response.scratch_response.
					scratch_status[i];
			if (req_pkp->query_request.select_criteria.
			    pool_criteria.
			    pool_count == 0)
			{
				scratch_statp->pool_id.pool = i;
			}
			else 
			{
				scratch_statp->pool_id = req_pkp->query_request.
							 select_criteria.pool_criteria.pool_id[i];
			}
			sprintf(scratch_statp->vol_id.external_label, 
				"PB03%02d", i);
			scratch_statp->home_location.panel_id.lsm_id.acs = (
				  ACS) i;
			scratch_statp->home_location.panel_id.lsm_id.lsm = (
				  LSM) i + 1;
			scratch_statp->home_location.panel_id.panel = (
				  PANEL) i + 2;
			scratch_statp->home_location.row = (ROW) i + 3;
			scratch_statp->home_location.col = (COL) i + 4;
			scratch_statp->status = STATUS_VOLUME_HOME;
			scratch_statp->media_type = i + 5;
		}
		*sizep = (char *)& query_resp->status_response.scratch_response.
			 scratch_status[query_resp->status_response.scratch_response.
			 volume_count] - (char *) query_resp;
		break;
	  case TYPE_SERVER:
		{
			QU_SRV_RESPONSE dummy[2];
			long		diff, diff2;

			query_resp->status_response.server_response.
			server_status.state = STATE_RUN;
			query_resp->status_response.server_response.
			server_status.freecells = 100;
			for (j = i = 0; i < 5; i++, j++)
			{
				query_resp->status_response.server_response.
				server_status.requests.requests[i][0]
				= j;
				query_resp->status_response.server_response.
				server_status.requests.requests[i][1]
				= j + 1;
			}
			*sizep = (char *)& query_resp->status_response.server_response.
				 server_status - (char *) query_resp;
			diff = (long)(&dummy[1]);
			diff2 = (long)(&dummy[0]);
			*sizep = *sizep + abs(diff - diff2);
		}
		break;
	  case TYPE_VOLUME:
		if (req_pkp->query_request.select_criteria.vol_criteria.
		    volume_count
		    == 0)
		{
			query_resp->status_response.volume_response.
			volume_count = MAX_ID;
		}
		else 
		{
			query_resp->status_response.volume_response.
			volume_count = 
				       req_pkp->query_request.select_criteria.vol_criteria.volume_count;
		}
		for (i = 0; i < query_resp->status_response.
			 volume_response.volume_count; i++)
		{
			volume_statp = &query_resp->status_response.volume_response.
				       volume_status[i];
			if (req_pkp->query_request.select_criteria.
			    vol_criteria.
			    volume_count == 0)
			{
				sprintf(tbuf, "SPE0%02d", i);
				strncpy(volume_statp->vol_id.
					external_label, tbuf, 
					EXTERNAL_LABEL_SIZE);
			}
			else 
			{
				volume_statp->vol_id = req_pkp->query_request.select_criteria.
						       vol_criteria.volume_id[i];
			}
			volume_statp->status = STATUS_VOLUME_IN_TRANSIT;
			volume_statp->location_type = LOCATION_CELL;
			volume_statp->location.cell_id.panel_id.lsm_id.acs = i;
			volume_statp->location.cell_id.panel_id.lsm_id.lsm = i + 1;
			volume_statp->location.cell_id.panel_id.panel = i + 2;
			volume_statp->location.cell_id.row = i + 3;
			volume_statp->location.cell_id.col = i + 4;
			volume_statp->media_type = i + 5;
		}
		*sizep = (char *)& query_resp->status_response.volume_response.
			 volume_status[query_resp->status_response.volume_response.
			 volume_count] - (char *) query_resp;
		break;
	  case TYPE_SUBPOOL_NAME:
		query_resp->status_response.
			    subpl_name_response.spn_status_count=19;
		for (i = 0; i < query_resp->status_response.subpl_name_response.
			    spn_status_count; i++) {
		    subpool_name_statp = &query_resp->
					 status_response.subpl_name_response.
					 subpl_name_status[i];
		    sprintf((char *) &subpool_name_statp->subpool_name, 
			     "SUBPOOLNAM%d", i);
		    subpool_name_statp->pool_id.pool = i;
		    subpool_name_statp->status = STATUS_SUCCESS;
		}

		*sizep = (char *)& query_resp->status_response.
			 subpl_name_response.subpl_name_status[query_resp->
			 status_response.subpl_name_response.spn_status_count] - 
			 (char *) query_resp;
		break;
	  case TYPE_DRIVE_GROUP:
		if (req_pkp->query_request.select_criteria.
		    drive_group_criteria.drg_count == 0) {
		    jlimit=7;
		    klimit=7;
		    llimit=10;
		}
		else {
		    jlimit=4;
		    klimit=2;
		    llimit=4;
		}
		drive_group_count= (jlimit) * (klimit) * (llimit);
		virt_drive_statp = &query_resp->status_response.
						drive_group_response;
		virt_drive_statp->vir_drv_map_count= drive_group_count;

		strncpy(virt_drive_statp->group_id.groupid, "GROUPID1", 
		       GROUPID_SIZE);
		virt_drive_statp->group_type=GROUP_TYPE_VTSS;
		i=0;
		for (j=0; j<jlimit; j++) {
		    virt_drive_statp->virt_drv_map[i].drive_id.panel_id.
				      lsm_id.acs= (ACS) j;
		    for (k=0; k < klimit; k++) {
			virt_drive_statp->virt_drv_map[i].drive_id.panel_id.
					  lsm_id.lsm=(LSM) k;
			for (l=0; l < llimit; l++) {
			    virt_drive_statp->virt_drv_map[i].drive_id.
					      panel_id.panel=(PANEL) 10;
			    virt_drive_statp->virt_drv_map[i].drive_id.
					      drive= (DRIVE) l;
			    virt_drive_statp->virt_drv_map[i].drive_addr=i;
			    i++;
			}
		    }
		}
		*sizep = (char *)& query_resp->
				    status_response.drive_group_response.
				    virt_drv_map[query_resp->status_response.
				    drive_group_response.vir_drv_map_count] - 
			 (char *) query_resp;
		break;
	  default:
		break;
	  }
}
static void
st_setsc_res(req_pkp, setsc_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
SET_SCRATCH_RESPONSE *setsc_resp;		/* set scratch response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter and index */

	setsc_resp->request_header = req_pkp->set_scratch_request.request_header;
	setsc_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(setsc_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	setsc_resp->message_status.status = STATUS_SUCCESS;
	setsc_resp->message_status.type = TYPE_NONE;
	setsc_resp->pool_id = req_pkp->set_scratch_request.pool_id;
	setsc_resp->count = req_pkp->set_scratch_request.count;
	*sizep += (char *)& setsc_resp->scratch_status[0]
	- (char *)& setsc_resp->request_header;
	for (i = 0; i < setsc_resp->count; i++)
	{
		setsc_resp->scratch_status[i].vol_id = 
						       req_pkp->set_scratch_request.vol_range[i].startvol;
		setsc_resp->scratch_status[i].status.status = STATUS_SUCCESS;
		setsc_resp->scratch_status[i].status.type = TYPE_POOL;
		if (req_pkp->set_scratch_request.pool_id.pool == 
		    SAME_POOL)
		{
			setsc_resp->scratch_status[i].status.identifier.pool_id.pool = 4;
		}
		else 
		{
			setsc_resp->scratch_status[i].status.identifier.pool_id = 
			  req_pkp->set_scratch_request.pool_id;
		}
		*sizep += (char *)& setsc_resp->scratch_status[1]
		- (char *)& setsc_resp->scratch_status[0];
	}
}
static void
st_setcl_res(req_pkp, setcl_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
SET_CLEAN_RESPONSE *setcl_resp;                 /* set clean response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter and index */

	setcl_resp->request_header = req_pkp->set_clean_request.request_header;
	setcl_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(setcl_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	setcl_resp->message_status.status = STATUS_SUCCESS;
	setcl_resp->message_status.type = TYPE_NONE;
	setcl_resp->max_use = req_pkp->set_clean_request.max_use;
	setcl_resp->count = req_pkp->set_clean_request.count;
	*sizep += (char *)& setcl_resp->volume_status[0]
	- (char *)& setcl_resp->request_header;
	for (i = 0; i < setcl_resp->count; i++)
	{
		setcl_resp->volume_status[i].vol_id = 
						      req_pkp->set_clean_request.vol_range[i].startvol;
		setcl_resp->volume_status[i].status.status = STATUS_SUCCESS;
		setcl_resp->volume_status[i].status.type = TYPE_NONE;
		*sizep += (char *)& setcl_resp->volume_status[1]
		- (char *)& setcl_resp->volume_status[0];
	}
}
/*
 * Name:        
 *      st_start_res()
 *
 * Description:
 *      Formats final response for an start command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_start_res(req_pkp, start_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
START_RESPONSE	*start_resp;			/* start response packet */
int		*sizep;

/* packet size returned */
{
	start_resp->request_header = req_pkp->generic_request;
	start_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(start_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	start_resp->message_status.status = STATUS_SUCCESS;
	start_resp->message_status.type = TYPE_NONE;
	*sizep = sizeof(START_RESPONSE);
}
/*
 * Name:        
 *      st_vary_res()
 *
 * Description:
 *      Formats final response for an vary command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_vary_res(req_pkp, vary_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
VARY_RESPONSE	*vary_resp;			/* vary response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;			/* loop counter and index */
	VA_ACS_STATUS	*acs_statp;		/* acs device status */
	VA_CAP_STATUS	*cap_statp;		/* cap device status */
	VA_LSM_STATUS	*lsm_statp;		/* lsm device status */
	VA_DRV_STATUS	*drive_statp;		/* drive device status */
	VA_PRT_STATUS	*port_statp;

	/* port device status */

	vary_resp->request_header = req_pkp->vary_request.request_header;
	vary_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(vary_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	vary_resp->message_status.status = STATUS_SUCCESS;
	vary_resp->message_status.type = TYPE_NONE;
	vary_resp->state = req_pkp->vary_request.state;
	vary_resp->type = req_pkp->vary_request.type;
	vary_resp->count = req_pkp->vary_request.count;
	for (i = 0; i < vary_resp->count; i++)
	{
		switch (req_pkp->vary_request.type)
		{
		  case TYPE_ACS:
			acs_statp = &vary_resp->device_status.acs_status[i];
			acs_statp->acs_id = req_pkp->vary_request.identifier.acs_id[i];
			acs_statp->status.status = STATUS_SUCCESS;
			acs_statp->status.type = TYPE_NONE;
			*sizep += sizeof(VA_ACS_STATUS);
			break;
		  case TYPE_CAP:
			cap_statp = &vary_resp->device_status.cap_status[i];
			cap_statp->cap_id = req_pkp->vary_request.identifier.cap_id[i];
			cap_statp->status.status = STATUS_SUCCESS;
			cap_statp->status.type = TYPE_NONE;
			*sizep += sizeof(VA_CAP_STATUS);
			break;
		  case TYPE_DRIVE:
			drive_statp = &vary_resp->device_status.drive_status[i];
			drive_statp->drive_id = req_pkp->vary_request.identifier.drive_id[i];
			drive_statp->status.status = STATUS_SUCCESS;
			drive_statp->status.type = TYPE_NONE;
			*sizep += sizeof(VA_DRV_STATUS);
			break;
		  case TYPE_LSM:
			lsm_statp = &vary_resp->device_status.lsm_status[i];
			lsm_statp->lsm_id = req_pkp->vary_request.identifier.lsm_id[i];
			lsm_statp->status.status = STATUS_SUCCESS;
			lsm_statp->status.type = TYPE_NONE;
			*sizep += sizeof(VA_LSM_STATUS);
			break;
		  case TYPE_PORT:
			port_statp = &vary_resp->device_status.port_status[i];
			port_statp->port_id = req_pkp->vary_request.identifier.port_id[i];
			port_statp->status.status = STATUS_SUCCESS;
			port_statp->status.type = TYPE_NONE;
			*sizep += sizeof(VA_PRT_STATUS);
			break;
		  default:
			break;
		  }
	}
	*sizep += (char *)& vary_resp->device_status
	- (char *)& vary_resp->request_header;
}
static void
st_def_pool_res(req_pkp, dp_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
DEFINE_POOL_RESPONSE *dp_rsp;			/* pool response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter and index */

	dp_rsp->request_header = req_pkp->define_pool_request.request_header;
	dp_rsp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(dp_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	dp_rsp->message_status.status = STATUS_SUCCESS;
	dp_rsp->message_status.type = TYPE_NONE;
	dp_rsp->low_water_mark = req_pkp->define_pool_request.low_water_mark;
	dp_rsp->high_water_mark = req_pkp->define_pool_request.high_water_mark;
	dp_rsp->pool_attributes = req_pkp->define_pool_request.pool_attributes;
	dp_rsp->count = req_pkp->define_pool_request.count;
	*sizep = (char *)& dp_rsp->pool_status[0]
		 - (char *)& dp_rsp->request_header;
	for (i = 0; i < dp_rsp->count; i++)
	{
		dp_rsp->pool_status[i].pool_id = req_pkp->define_pool_request.pool_id[i];
		dp_rsp->pool_status[i].pool_id = req_pkp->define_pool_request.pool_id[i];
		dp_rsp->pool_status[i].status.type = TYPE_NONE;
		dp_rsp->pool_status[i].status.status = STATUS_POOL_LOW_WATER;
		*sizep += (char *)& dp_rsp->pool_status[1]
		- (char *)& dp_rsp->pool_status[0];
	}
}
static void
st_del_pool_res(req_pkp, dp_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
DELETE_POOL_RESPONSE *dp_rsp;			/* delete pool response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter and index */

	dp_rsp->request_header = req_pkp->delete_pool_request.request_header;
	dp_rsp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(dp_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	dp_rsp->message_status.status = STATUS_SUCCESS;
	dp_rsp->message_status.type = TYPE_NONE;
	if (req_pkp->delete_pool_request.count == 0)
	{
		dp_rsp->count = MAX_ID;
	}
	else 
	{
		dp_rsp->count = req_pkp->delete_pool_request.count;
	}
	*sizep += (char *)& dp_rsp->pool_status[0] - (char *)& dp_rsp->request_header;
	for (i = 0; i < dp_rsp->count; i++)
	{
		if (req_pkp->delete_pool_request.count == 0)
		{
			dp_rsp->pool_status[i].pool_id.pool = (POOL) i + 1;
		}
		else 
		{
			dp_rsp->pool_status[i].pool_id = req_pkp->delete_pool_request.pool_id[i];
		}
		dp_rsp->pool_status[i].status.status = STATUS_SUCCESS;
		dp_rsp->pool_status[i].status.type = TYPE_NONE;
		*sizep += (char *)& dp_rsp->pool_status[1]
		- (char *)& dp_rsp->pool_status[0];
	}
}
/*
 * Name:        
 *      st_unlock_res()
 *
 * Description:
 *      Formats final response for an unlock command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_unlock_res(req_pkp, unl_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
UNLOCK_RESPONSE *unl_rsp;			/* unlock response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;			/* loop counter and index */
	LO_DRV_STATUS	*drive_statp;		/* drive status */
	LO_VOL_STATUS	*vol_statp;

	/* volume status */

	unl_rsp->request_header = req_pkp->unlock_request.request_header;
	unl_rsp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(unl_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	unl_rsp->message_status.status = STATUS_SUCCESS;
	unl_rsp->message_status.type = TYPE_NONE;
	unl_rsp->type = req_pkp->unlock_request.type;
	if (req_pkp->unlock_request.count == 0)
	{
		unl_rsp->count = MAX_ID;
	}
	else 
	{
		unl_rsp->count = req_pkp->unlock_request.count;
	}
	*sizep = (char *)& unl_rsp->identifier_status
		 - (char *)& unl_rsp->request_header;
	for (i = 0; i < unl_rsp->count; i++)
	{
		switch (req_pkp->unlock_request.type)
		{
		  case TYPE_DRIVE:
			drive_statp = &unl_rsp->identifier_status.drive_status[i];
			if (req_pkp->unlock_request.count == 0)
			{
				drive_statp->drive_id.panel_id.lsm_id.acs = i;
				drive_statp->drive_id.panel_id.lsm_id.lsm = i + 1;
				drive_statp->drive_id.panel_id.panel = 10;
				drive_statp->drive_id.drive = 3;
			}
			else 
			{
				drive_statp->drive_id = req_pkp->unlock_request.identifier.drive_id[i];
			}
			drive_statp->status.status = STATUS_SUCCESS;
			drive_statp->status.type = TYPE_NONE;
			*sizep += sizeof(LO_DRV_STATUS);
			break;
		  case TYPE_VOLUME:
			vol_statp = &unl_rsp->identifier_status.volume_status[i];
			if (req_pkp->unlock_request.count == 0)
			{
				sprintf(vol_statp->vol_id.
					external_label, "PB03%02d", i);
			}
			else 
			{
				vol_statp->vol_id = req_pkp->unlock_request.identifier.vol_id[i];
			}
			vol_statp->status.status = STATUS_SUCCESS;
			vol_statp->status.type = TYPE_NONE;
			*sizep += sizeof(LO_VOL_STATUS);
			break;
		  default:
			break;
		  }
	}
}
/*
 * Name:        
 *      st_lock_res()
 *
 * Description:
 *      Formats final response for an lock command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_lock_res(req_pkp, lock_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
LOCK_RESPONSE	*lock_rsp;			/* lock response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;			/* loop counter and index */
	LO_DRV_STATUS	*drive_statp;		/* drive status */
	LO_VOL_STATUS	*vol_statp;

	/* volume status */

	lock_rsp->request_header = req_pkp->lock_request.request_header;
	lock_rsp->request_header.ipc_header.module_type = TYPE_LM;
	lock_rsp->request_header.message_header.lock_id = (unsigned int)9999;
	strcpy(lock_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	lock_rsp->message_status.type = TYPE_NONE;
	lock_rsp->type = req_pkp->lock_request.type;
	lock_rsp->count = req_pkp->lock_request.count;
	lock_rsp->message_status.status = STATUS_SUCCESS;
	*sizep = (char *)& lock_rsp->identifier_status
		 - (char *)& lock_rsp->request_header;
	for (i = 0; i < lock_rsp->count; i++)
	{
		switch (req_pkp->lock_request.type)
		{
		  case TYPE_VOLUME:
			vol_statp = &lock_rsp->identifier_status.volume_status[i];
			vol_statp->vol_id = req_pkp->lock_request.identifier.vol_id[i];
			if (i != 0)
			{
				vol_statp->status.status = STATUS_SUCCESS;
				vol_statp->status.type = TYPE_NONE;
			}
			else 
			{
				vol_statp->status.status = STATUS_DEADLOCK;
				vol_statp->status.identifier.vol_id = 
				  req_pkp->lock_request.identifier.vol_id[lock_rsp->count - 1];
				vol_statp->status.type = TYPE_VOLUME;
			}
			*sizep += sizeof(LO_VOL_STATUS);
			break;
		  case TYPE_DRIVE:
			drive_statp = &lock_rsp->identifier_status.drive_status[i];
			drive_statp->drive_id = req_pkp->lock_request.identifier.drive_id[i];
			if (i != 0)
			{
				drive_statp->status.status = STATUS_SUCCESS;
				drive_statp->status.type = TYPE_NONE;
			}
			else 
			{
				drive_statp->status.status = STATUS_DEADLOCK;
				drive_statp->status.identifier.drive_id = 
				  req_pkp->lock_request.identifier.drive_id[lock_rsp->count - 1];
				drive_statp->status.type = TYPE_DRIVE;
			}
			*sizep += sizeof(LO_DRV_STATUS);
			break;
		  default:
			break;
		  }
	}
}
/*
 * Name:        
 *      st_clear_lock_res()
 *
 * Description:
 *      Formats final response for an lock command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_clearlock_res(req_pkp, clrlock_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
CLEAR_LOCK_RESPONSE *clrlock_rsp;		/* clear_lock response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;			/* loop counter and index */
	LO_DRV_STATUS	*drive_statp;		/* drive status */
	LO_VOL_STATUS	*vol_statp;

	/* volume status */

	clrlock_rsp->request_header = req_pkp->lock_request.request_header;
	clrlock_rsp->request_header.message_header.lock_id = NO_LOCK_ID;
	clrlock_rsp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(clrlock_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	clrlock_rsp->message_status.status = STATUS_SUCCESS;
	clrlock_rsp->message_status.type = TYPE_NONE;
	clrlock_rsp->type = req_pkp->lock_request.type;
	clrlock_rsp->count = req_pkp->lock_request.count;
	*sizep = (char *)& clrlock_rsp->identifier_status
		 - (char *)& clrlock_rsp->request_header;
	for (i = 0; i < clrlock_rsp->count; i++)
	{
		switch (req_pkp->lock_request.type)
		{
		  case TYPE_DRIVE:
			drive_statp = &clrlock_rsp->identifier_status.drive_status[i];
			drive_statp->drive_id = req_pkp->lock_request.identifier.drive_id[i];
			drive_statp->status.status = STATUS_SUCCESS;
			drive_statp->status.type = TYPE_NONE;
			*sizep += sizeof(LO_DRV_STATUS);
			break;
		  case TYPE_VOLUME:
			vol_statp = &clrlock_rsp->identifier_status.volume_status[i];
			vol_statp->vol_id = req_pkp->lock_request.identifier.vol_id[i];
			vol_statp->status.status = STATUS_SUCCESS;
			vol_statp->status.type = TYPE_NONE;
			*sizep += sizeof(LO_VOL_STATUS);
			break;
		  default:
			break;
		  }
	}
}
/*
 * Name:        
 *      st_querylock_res()
 *
 * Description:
 *      Formats final response for an query lock command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_querylock_res(req_pkp, qul_rsp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
QUERY_LOCK_RESPONSE *qul_rsp;			/* clear_lock response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;			/* loop counter and index */
	QL_DRV_STATUS	*drive_statp;		/* drive status */
	QL_VOL_STATUS	*vol_statp;

	/* volume status */

	qul_rsp->request_header = req_pkp->query_lock_request.request_header;
	qul_rsp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(qul_rsp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	qul_rsp->message_status.status = STATUS_SUCCESS;
	qul_rsp->message_status.type = TYPE_NONE;
	qul_rsp->type = req_pkp->query_lock_request.type;
	if (req_pkp->query_lock_request.count == 0)
	{
		qul_rsp->count = MAX_ID;
	}
	else 
	{
		qul_rsp->count = req_pkp->query_lock_request.count;
	}
	*sizep = (char *)& qul_rsp->identifier_status
		 - (char *)& qul_rsp->request_header;
	for (i = 0; i < qul_rsp->count; i++)
	{
		switch (req_pkp->query_lock_request.type)
		{
		  case TYPE_DRIVE:
			drive_statp = &qul_rsp->identifier_status.drive_status[i];
			if (req_pkp->query_lock_request.count == 0)
			{
				drive_statp->drive_id.panel_id.lsm_id.acs = i;
				drive_statp->drive_id.panel_id.lsm_id.lsm = i + 1;
				drive_statp->drive_id.panel_id.panel = 10;
				drive_statp->drive_id.drive = 3;
			}
			else 
			{
				drive_statp->drive_id = req_pkp->query_lock_request.identifier.drive_id[i];
			}
			drive_statp->lock_id = 99;
			drive_statp->lock_duration = 9999;
			drive_statp->locks_pending = 9;
			drive_statp->status = STATUS_DRIVE_AVAILABLE;
			strcpy(drive_statp->user_id.user_label, 
			       "Ingemar");
			*sizep += sizeof(QL_DRV_STATUS);
			break;
		  case TYPE_VOLUME:
			vol_statp = &qul_rsp->identifier_status.volume_status[i];
			if (req_pkp->query_lock_request.count == 0)
			{
				sprintf(vol_statp->vol_id.
					external_label, "PB03%02d", i);
			}
			else 
			{
				vol_statp->vol_id = req_pkp->query_lock_request.identifier.vol_id[i];
			}
			vol_statp->lock_id = 99;
			vol_statp->lock_duration = 9999;
			vol_statp->locks_pending = 9;
			vol_statp->status = STATUS_VOLUME_AVAILABLE;
			strcpy(vol_statp->user_id.user_label, "Pirmin");
			*sizep += sizeof(QL_VOL_STATUS);
			break;
		  default:
			break;
		  }
	}
}
/*
 * Name:        
 *      st_venter_res()
 *
 * Description:
 *      Formats final response for an venter command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_venter_res(req_pkp, venter_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
ENTER_RESPONSE	*venter_resp;			/* venter response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter */

	venter_resp->request_header = req_pkp->generic_request;
	venter_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(venter_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	venter_resp->message_status.status = STATUS_SUCCESS;
	venter_resp->message_status.type = TYPE_NONE;
	venter_resp->cap_id = req_pkp->venter_request.cap_id;
	venter_resp->count = req_pkp->venter_request.count;
	for (i = 0; i < venter_resp->count; i++)
	{
		venter_resp->volume_status[i].vol_id = req_pkp->venter_request.vol_id[i];
		venter_resp->volume_status[i].status.status = STATUS_SUCCESS;
		venter_resp->volume_status[i].status.type = TYPE_CAP;
		venter_resp->volume_status[i].status.identifier.cap_id = 
		  req_pkp->enter_request.cap_id;
	}
	*sizep = (char *)& venter_resp->volume_status[venter_resp->count]
		 - (char *) venter_resp;
}
/*
 * Name:        
 *      st_setcap_res()
 *
 * Description:
 *      Formats final response for a set cap command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_setcap_res(req_pkp, setcap_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
SET_CAP_RESPONSE *setcap_resp;			/* setcap response packet */
int		*sizep;                         /* packet size returned */
{
	unsigned short	i;

	/* loop counter */

	setcap_resp->request_header = req_pkp->generic_request;
	setcap_resp->request_header.ipc_header.module_type = TYPE_LM;
	strcpy(setcap_resp->request_header.ipc_header.return_socket, 
	       my_sock_name);
	setcap_resp->message_status.status = STATUS_SUCCESS;
	setcap_resp->message_status.type = TYPE_NONE;
	setcap_resp->cap_priority = req_pkp->set_cap_request.cap_priority;
	setcap_resp->cap_mode = req_pkp->set_cap_request.cap_mode;
	setcap_resp->count = req_pkp->set_cap_request.count;
	*sizep = (char *)& setcap_resp->set_cap_status[0]
		 - (char *)& setcap_resp->request_header;
	for (i = 0; i < setcap_resp->count; i++)
	{
		setcap_resp->set_cap_status[i].cap_id = req_pkp->set_cap_request.cap_id[i];
		setcap_resp->set_cap_status[i].status.status = STATUS_SUCCESS;
		setcap_resp->set_cap_status[i].status.type = TYPE_NONE;
		*sizep = (char *)& setcap_resp->set_cap_status[1]
			 - (char *)& setcap_resp->set_cap_status[0];
	}
	*sizep = (char *)& setcap_resp->set_cap_status[setcap_resp->count]
		 - (char *) setcap_resp;
}
/*
 * Name:        
 *      st_unregister_res()
 *
 * Description:
 *      Formats final response for an unregister command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_unregister_res(req_pkp, unregister_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
UNREGISTER_RESPONSE *unregister_resp;		/* unregister response packet */
int		*sizep;                         /* packet size returned */
{

  unregister_resp->request_header = req_pkp->generic_request;
  unregister_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(unregister_resp->request_header.ipc_header.return_socket,my_sock_name);
  unregister_resp->message_status.status = STATUS_SUCCESS;
  unregister_resp->message_status.type = TYPE_NONE;
  strcpy(unregister_resp->event_register_status.registration_id.registration, "1234567890123456789012345678901");
  unregister_resp->event_register_status.count=3;

  unregister_resp->event_register_status.register_status[0].event_class = EVENT_CLASS_VOLUME;
  unregister_resp->event_register_status.register_status[0].register_return = EVENT_REGISTER_UNREGISTERED;
  unregister_resp->event_register_status.register_status[1].event_class = EVENT_CLASS_RESOURCE;
  unregister_resp->event_register_status.register_status[1].register_return = EVENT_REGISTER_UNREGISTERED;
  unregister_resp->event_register_status.register_status[2].event_class = EVENT_CLASS_DRIVE_ACTIVITY;
  unregister_resp->event_register_status.register_status[2].register_return = EVENT_REGISTER_UNREGISTERED;

  *sizep = (char *)& unregister_resp->event_register_status.register_status
    [unregister_resp->event_register_status.count] - (char *) unregister_resp;
}
/*
 * Name:        
 *      st_check_registration_res()
 *
 * Description:
 *      Formats final response for a check_registration command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_check_registration_res(req_pkp, check_registration_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
CHECK_REGISTRATION_RESPONSE *check_registration_resp;/* check_registration response packet */
int		*sizep;                         /* packet size returned */
{

  check_registration_resp->request_header = req_pkp->generic_request;
  check_registration_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(check_registration_resp->request_header.ipc_header.return_socket, my_sock_name);
  check_registration_resp->message_status.status = STATUS_SUCCESS;
  check_registration_resp->message_status.type = TYPE_NONE;
  strcpy(check_registration_resp->event_register_status.registration_id.
     registration,"abcdefghijklmnopqrstuvwxyz12345");
  check_registration_resp->event_register_status.count=3;

  check_registration_resp->event_register_status.register_status[0].event_class = EVENT_CLASS_VOLUME;
  check_registration_resp->event_register_status.register_status[0].register_return = EVENT_REGISTER_REGISTERED;
  check_registration_resp->event_register_status.register_status[1].event_class = EVENT_CLASS_RESOURCE;
  check_registration_resp->event_register_status.register_status[1].register_return = EVENT_REGISTER_REGISTERED;
  check_registration_resp->event_register_status.register_status[2].event_class = EVENT_CLASS_DRIVE_ACTIVITY;
  check_registration_resp->event_register_status.register_status[2].register_return = EVENT_REGISTER_REGISTERED;

  *sizep = (char *)& check_registration_resp->event_register_status.register_status
    [check_registration_resp->event_register_status.count] - (char *) check_registration_resp;
}
/*
 * Name:        
 *      st_register_reg_event_intermed_res()
 *
 * Description:
 *      Formats a register_event response for a register command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_register_reg_event_intermed_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* unregister response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_REGISTER;
  register_resp->event_sequence = 300031;
  strcpy(register_resp->event.event_register_status.registration_id.registration,"ABCDEGHIJKLMNOPSQRSTUVWXYZ????:");
  register_resp->event.event_register_status.count=3;
  register_resp->event.event_register_status.register_status[0].event_class = EVENT_CLASS_VOLUME;
  register_resp->event.event_register_status.register_status[1].event_class = EVENT_CLASS_RESOURCE;
  register_resp->event.event_register_status.register_status[2].event_class = EVENT_CLASS_DRIVE_ACTIVITY;
  register_resp->event.event_register_status.register_status[0].register_return = EVENT_REGISTER_REGISTERED;
  register_resp->event.event_register_status.register_status[1].register_return = EVENT_REGISTER_REGISTERED;
  register_resp->event.event_register_status.register_status[2].register_return = EVENT_REGISTER_REGISTERED;
  *sizep = (char *)&register_resp->event.event_register_status.register_status
  [register_resp->event.event_register_status.count] - (char *) register_resp;
}
/*
 * Name:        
 *      st_register_vol_event_res()
 *
 * Description:
 *      Formats a vol_event response for a register command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_register_vol_event_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_VOLUME;
  register_resp->event_sequence = 300099;
  register_resp->event.event_volume_status.event_type = VOL_DELETED;
  strcpy(register_resp->event.event_volume_status.vol_id.external_label, "CHRISH");
  *sizep = sizeof(EVENT_VOLUME_STATUS) + (char *)&register_resp->event - (char *) register_resp;
}
/*
 * Name:        
 *      st_register_resource_event_res()
 *
 * Description:
 *      Formats a resource_event response for a register command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_register_resource_event_hli_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 999999;
  register_resp->event.event_resource_status.resource_type = TYPE_DRIVE;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.acs = 12;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.lsm = 12;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.panel = 12;
  register_resp->event.event_resource_status.resource_identifier.drive_id.drive = 1;
  register_resp->event.event_resource_status.resource_event = RESOURCE_UNIT_ATTENTION;
  register_resp->event.event_resource_status.resource_data_type = SENSE_TYPE_HLI;
  register_resp->event.event_resource_status.resource_data.sense_hli.category = 49;
  register_resp->event.event_resource_status.resource_data.sense_hli.code = 72;
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_scsi_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 888888;
  register_resp->event.event_resource_status.resource_type = TYPE_DRIVE;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.acs = 11;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.lsm = 11;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.panel = 11;
  register_resp->event.event_resource_status.resource_identifier.drive_id.drive = 2;
  register_resp->event.event_resource_status.resource_event = RESOURCE_HARDWARE_ERROR;
  register_resp->event.event_resource_status.resource_data_type = SENSE_TYPE_SCSI;
  register_resp->event.event_resource_status.resource_data.sense_scsi.sense_key = 69;
  register_resp->event.event_resource_status.resource_data.sense_scsi.asc = 30;
  register_resp->event.event_resource_status.resource_data.sense_scsi.ascq = 33;
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_fsc_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 777777;
  register_resp->event.event_resource_status.resource_type = TYPE_HAND;
  register_resp->event.event_resource_status.resource_identifier.hand_id.panel_id.lsm_id.acs = 0;
  register_resp->event.event_resource_status.resource_identifier.hand_id.panel_id.lsm_id.lsm = 1;
  register_resp->event.event_resource_status.resource_identifier.hand_id.panel_id.panel = 9;
  register_resp->event.event_resource_status.resource_identifier.hand_id.hand = 3;
  register_resp->event.event_resource_status.resource_event = RESOURCE_DEGRADED_MODE;
  register_resp->event.event_resource_status.resource_data_type = SENSE_TYPE_FSC;
  strncpy(register_resp->event.event_resource_status.resource_data.sense_fsc.fsc,"badd",4);
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_lsmser_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 666666;
  register_resp->event.event_resource_status.resource_type = TYPE_LSM;
  register_resp->event.event_resource_status.resource_identifier.lsm_id.acs = 7;
  register_resp->event.event_resource_status.resource_identifier.lsm_id.lsm = 4;
  register_resp->event.event_resource_status.resource_event = RESOURCE_SERIAL_NUM_CHG;
  register_resp->event.event_resource_status.resource_data_type = RESOURCE_CHANGE_SERIAL_NUM;
  strncpy(register_resp->event.event_resource_status.resource_data.serial_num.serial_nbr,"123123123",9);
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_lsm_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 555555;
  register_resp->event.event_resource_status.resource_type = TYPE_LSM;
  register_resp->event.event_resource_status.resource_identifier.lsm_id.acs = 127;
  register_resp->event.event_resource_status.resource_identifier.lsm_id.lsm = 24;
  register_resp->event.event_resource_status.resource_event = RESOURCE_LSM_TYPE_CHG;
  register_resp->event.event_resource_status.resource_data_type = RESOURCE_CHANGE_LSM_TYPE;
  register_resp->event.event_resource_status.resource_data.lsm_type = 9999;
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_drv_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* register response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_RESOURCE;
  register_resp->event_sequence = 444444;
  register_resp->event.event_resource_status.resource_type = TYPE_DRIVE;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.acs = 127;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.lsm_id.lsm = 24;
  register_resp->event.event_resource_status.resource_identifier.drive_id.panel_id.panel = 19;
  register_resp->event.event_resource_status.resource_identifier.drive_id.drive = 3;
  register_resp->event.event_resource_status.resource_event = RESOURCE_DRIVE_TYPE_CHG;
  register_resp->event.event_resource_status.resource_data_type = RESOURCE_CHANGE_DRIVE_TYPE;
  register_resp->event.event_resource_status.resource_data.drive_type = 13;
  *sizep = (sizeof(EVENT_RESOURCE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void     
st_register_resource_event_mount_drv_act(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;                          /* request packet */
REGISTER_RESPONSE *register_resp;               /* register response packet */
int             *sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_DRIVE_ACTIVITY;
  register_resp->event_sequence = 444445;
  register_resp->event.event_drive_status.event_type = TYPE_MOUNT;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.start_time = 536872240;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.completion_time = 536872245;
  strncpy(register_resp->event.event_drive_status.resource_data.drive_activity_data.vol_id.external_label, "PB0302", 6);
  register_resp->event.event_drive_status.resource_data.drive_activity_data.volume_type = VOLUME_TYPE_DATA;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.lsm_id.acs = 127;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.lsm_id.lsm = 24;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.panel = 19;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.drive = 3;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.pool_id.pool = 10;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.lsm_id.acs = 127;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.lsm_id.lsm = 24;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.panel = 19;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.row = 20;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.col = 10;

  register_resp->event.event_drive_status.resource_data_type = DRIVE_ACTIVITY_DATA_TYPE;
  *sizep = (sizeof(EVENT_DRIVE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
static void
st_register_resource_event_dismount_drv_act(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;                          /* request packet */
REGISTER_RESPONSE *register_resp;               /* register response packet */
int             *sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->request_header.message_header.message_options |= INTERMEDIATE;
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_DRIVE_ACTIVITY;
  register_resp->event_sequence = 444446;
  register_resp->event.event_drive_status.event_type = TYPE_DISMOUNT;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.start_time = 536872250;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.completion_time = 536872255;
  strncpy(register_resp->event.event_drive_status.resource_data.drive_activity_data.vol_id.external_label, "PB0305", 6);
  register_resp->event.event_drive_status.resource_data.drive_activity_data.volume_type = VOLUME_TYPE_CLEAN;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.lsm_id.acs = 127;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.lsm_id.lsm = 24;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.panel_id.panel = 19;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.drive_id.drive = 3;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.pool_id.pool = 10;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.lsm_id.acs = 127;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.lsm_id.lsm = 24;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.panel_id.panel = 19;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.row = 20;
  register_resp->event.event_drive_status.resource_data.drive_activity_data.home_location.col = 10;

  register_resp->event.event_drive_status.resource_data_type = DRIVE_ACTIVITY_DATA_TYPE;
  *sizep = (sizeof(EVENT_DRIVE_STATUS) + (char *)& register_resp->event) - (char *) register_resp;
}
/*
 * Name:        
 *      st_register_reg_event_final_res()
 *
 * Description:
 *      Formats a register_event final response for a register command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_register_reg_event_final_res(req_pkp, register_resp, sizep)
REQUEST_TYPE *req_pkp;				/* request packet */
REGISTER_RESPONSE *register_resp;		/* unregister response packet */
int		*sizep;                         /* packet size returned */
{
  register_resp->request_header = req_pkp->generic_request;
  register_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(register_resp->request_header.ipc_header.return_socket,my_sock_name);
  register_resp->message_status.status = STATUS_SUCCESS;
  register_resp->message_status.type = TYPE_NONE;
  register_resp->event_reply_type = EVENT_REPLY_REGISTER;
  register_resp->event_sequence = 300031;
  strcpy(register_resp->event.event_register_status.registration_id.registration,"12345ABCDEGHIJKLMNOPSQRSTUVWXYZ");
  register_resp->event.event_register_status.count=3;
  register_resp->event.event_register_status.register_status[0].event_class = EVENT_CLASS_VOLUME;
  register_resp->event.event_register_status.register_status[1].event_class = EVENT_CLASS_RESOURCE;
  register_resp->event.event_register_status.register_status[2].event_class = EVENT_CLASS_DRIVE_ACTIVITY;
  register_resp->event.event_register_status.register_status[0].register_return = EVENT_REGISTER_REGISTERED;
  register_resp->event.event_register_status.register_status[1].register_return = EVENT_REGISTER_REGISTERED;
  register_resp->event.event_register_status.register_status[2].register_return = EVENT_REGISTER_REGISTERED;
  *sizep = (char *)& register_resp->event.event_register_status.register_status
  [register_resp->event.event_register_status.count] - (char *) register_resp;
}
/*
 * Name:        
 *      st_display_res()
 *
 * Description:
 *      Formats a display final response for a display command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void	
st_display_res(REQUEST_TYPE *req_pkp, 
	       DISPLAY_RESPONSE *display_resp, 
	       int *sizep)
{
  display_resp->request_header = req_pkp->generic_request;
  display_resp->request_header.ipc_header.module_type = TYPE_LM;
  strcpy(display_resp->request_header.ipc_header.return_socket,my_sock_name);
  display_resp->message_status.status = STATUS_SUCCESS;
  display_resp->message_status.type = TYPE_NONE;
  display_resp->display_type = TYPE_NONE;
  display_resp->display_xml_data.length = 3626;
  strcpy(display_resp->display_xml_data.xml_data,
"<response type=\"display\"><display seq=\"1\" type=\"initial\"><type name=\"panel\"/> \
<format><fields><field name =\"acs\" format=\"int\" maxlen =\"3\"/><field name=\"lsm\"\
format=\"int\" maxlen=\"3\"/><field name=\"panel\" format=\"int\" maxlen=\"5\"/>\
<field name=\"type\" format=\"int\" maxlen=\"4\"/></fields></format></display>\
</response><response type=\"display\"><display seq=\"2\" type=\"intermediate\">\
<data><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">0</f>\
<fmaxlen=\"4\">8</f></r><r><f maxlen=\"3\">0</f><f maxlen =\"3\">0</f><f maxlen=\"5\">\
1< /f><f maxlen=\"4\">5</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f>\
<f maxlen=\"5\">2</f><f maxlen=\"4\">1</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">\
0</f><fmax len=\"5\">3</f><f maxlen=\"4\">4</f></r><r><f maxlen=\"3\">0</f>\
<f maxlen=\"3\">0</f><f maxlen=\"5\">4</f><f maxlen=\"4\">1</f></r><r><f maxlen=\"3\">\
0</f><f maxlen=\"3\">0 </f><f maxlen=\"5\">5</f><f maxlen=\"4\">1</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">6</f><f maxlen=\"4\">1\
</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">7</f>\
<f maxlen=\"4\">13</f ></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f>\
<f maxlen=\"5\">8</f><f maxlen=\"4\">6</f></r><r><f maxlen=\"3\">0</f>\
<f maxlen=\"3\">0</f><f maxlen=\"5\">9</f><f maxlen=\"4\">6</f></r><r><f maxlen=\"3\">\
0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">10</f><f maxlen=\"4\">7</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">11 </f><f maxlen=\"4\">\
9</f></r ><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen= \"5\">12</f>\
<f maxlen=\"4\">12 </f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f>\
<f maxlen=\"5\">13</f><f maxlen =\"4\">10</f></r><r><f maxl n=\"3\">0</f>\
<f maxlen=\"3\">0 </f><f maxlen=\"5\">14</f><f maxlen=\"4\">10</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">1 5</f><f maxlen=\"4\">\
10</f>< /r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">16\
</f><f maxlen=\"4\">10</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f>\
<f maxlen=\"5\">17</f><f maxlen=\"4\">10</f></r><r><f maxlen=\"3\">0</f>\
<f maxlen=\"3\">0</f><f maxlen=\"5\">18</f><f maxlen=\"4\">10</f></r>\
<r><f maxlen=\"3\">0</f><f maxlen=\"3\">0</f><f maxlen=\"5\">19</f><f maxlen=\"4\">\
11</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">0</f>\
<f maxlen=\"4\">8</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f>\
<f maxlen=\"5\">1</f><f maxlen=\"4\">5</f></r><r><f maxlen=\"3\">0</f>\
<f maxlen=\"3\">1</f><f maxlen=\"5\">2</f><f maxlen=\"4\">1</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">3</f><f maxlen=\"4\">\
4</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">4</f>\
<f maxlen=\"4\">1</f> </r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f>\
<f maxlen=\"5\">5</f><f maxlen=\"4\">5</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1\
</f><f maxlen=\"5\">6</f><f maxlen=\"4\">1</f></r><r><f maxlen =\"3\">0</f>\
<f maxlen=\"3\">1</f><f maxlen=\"5\">7</f><f maxlen=\"4\">4</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">8</f><f maxlen=\"4\">\
1</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">9\
</f><f maxlen=\"4\">13</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f>\
<f maxlen=\"5\">10</f><f maxlen=\"4\">14</f></r><r><f maxlen=\"3\">0</f>\
<f maxlen=\"3\">1</f><f maxlen=\"5\">11</f><f maxlen=\"4\">9</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">12</f><f maxlen=\"4\">\
12</f></r></data></display></response>=\"4\">1</f></r><r><f maxlen=\"3\">0\
</f><f maxlen=\"3\">1</f><f maxlen=\"5\">9</f><f maxlen=\"4\">13</f></r><r>\
<f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">10</f><f maxlen=\"4\">\
14</f></r><r><f maxlen=\"3\">0</f><f maxlen=\"3\">1</f><f maxlen=\"5\">11\
</f><f maxlen=\"4\">9</f></r><r><f maxlen=\"3\">0<xml_data> for testing \
purposes this is the end of the data stream for the t_cdriver tests.  Under \
normal server behavior the stream will end with an xml tag.");

  *sizep = (char *) &display_resp->display_xml_data +sizeof(unsigned short) + 
	   display_resp->display_xml_data.length - (char *) display_resp;
}
/*
 * Name:        
 *      st_mount_pinfo_res()
 *
 * Description:
 *      Formats a mount_pinfo response for a mount_pinfo command.
 *
 * Return Values
 *      NONE
 *
 * Considerations
 *      NONE
 */
static void
st_mount_pinfo_res(REQUEST_TYPE *req_pkp, 
		     MOUNT_PINFO_RESPONSE *mount_pinfo_resp, 
		     int *sizep)
{
    mount_pinfo_resp->request_header = req_pkp->generic_request;
    mount_pinfo_resp->request_header.ipc_header.module_type = TYPE_LM;
    strcpy(mount_pinfo_resp->request_header.ipc_header.return_socket, 
	   my_sock_name);
    mount_pinfo_resp->message_status.status = STATUS_SUCCESS;
    mount_pinfo_resp->message_status.type = TYPE_NONE;
    mount_pinfo_resp->pool_id = req_pkp->mount_pinfo_request.pool_id;
    mount_pinfo_resp->drive_id = req_pkp->mount_pinfo_request.drive_id;
    strncpy(mount_pinfo_resp->vol_id.external_label, 
	    req_pkp->mount_pinfo_request.vol_id.external_label, 
	    EXTERNAL_LABEL_SIZE);
    *sizep = sizeof(MOUNT_PINFO_RESPONSE);
}
