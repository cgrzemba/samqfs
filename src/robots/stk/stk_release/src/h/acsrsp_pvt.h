/* SccsId @(#)acsrsp_pvt.h	2.2 10/31/01 (c) 1992-2001 STK */
#ifndef _ACSRSP_PVT_
/**********************************************************************
**   Copyright StorageTek, 2001
**
**   Name:     acsrsp.pvt
**
**   Purpose:  This is the acs_response private header file. 
**             Used by the ACS100 and ACS200 level functions.
**
**
**---------------------------------------------------------------------
**   Maintenance History:
**
**   07/09/93: KJS Created from HSC land (Tom Rethard)
**   10/15/01: Scott Siao    Added support for register, unregister and
**                           check_registration commands.
**   11/12/01: Scott Siao    Added support for display command.
**   02/06/02: Scott Siao    Added support for query_drive_group,
**                           query_subpool_name, and virtual_mount
**                           commands/responses.
**
***********************************************************************
*/

/*---------------------------------------------------------------------
**   Required header files
**---------------------------------------------------------------------
*/

#define _ACSRSP_PVT_ 1
/* Global and Static Variable Declarations: */

typedef union 
  {
  ACS_AUDIT_ACS_RESPONSE *       haAAR;
  ACS_AUDIT_INT_RESPONSE *       haAIR;
  ACS_AUDIT_LSM_RESPONSE *       haALR;
  ACS_AUDIT_PNL_RESPONSE *       haAPR;
  ACS_AUDIT_SUB_RESPONSE *       haASPR;
  ACS_AUDIT_SRV_RESPONSE *       haASRR;
  ACS_CANCEL_RESPONSE *          haCR;
  ACS_CLEAR_LOCK_DRV_RESPONSE *  haCLDR;
  ACS_CLEAR_LOCK_VOL_RESPONSE *  haCLVR;
  ACS_DEFINE_POOL_RESPONSE *     haDFPR;
  ACS_DELETE_POOL_RESPONSE *     haDLPR;
  ACS_DISMOUNT_RESPONSE *        haDR;
  ACS_EJECT_RESPONSE *           haEJR;
  ACS_ENTER_RESPONSE *           haENR;
  ACS_IDLE_RESPONSE *            haIR;
  ACS_LOCK_DRV_RESPONSE *        haLDR;
  ACS_LOCK_VOL_RESPONSE *        haLVR;
  ACS_MOUNT_RESPONSE *           haMR;
  ACS_MOUNT_SCRATCH_RESPONSE *   haMSR;
  ACS_QUERY_ACS_RESPONSE *       haQACSR;
  ACS_QUERY_CAP_RESPONSE *       haQCR;
  ACS_QUERY_CLN_RESPONSE *       haQCLR;
  ACS_QUERY_DRV_RESPONSE *       haQDR;
  ACS_QUERY_LOCK_DRV_RESPONSE *  haQLDR;
  ACS_QUERY_LOCK_VOL_RESPONSE *  haQLVR;
  ACS_QUERY_LSM_RESPONSE *       haQLR;
  ACS_QUERY_MMI_RESPONSE *       haQMMR;
  ACS_QUERY_MNT_RESPONSE *       haQMR;
  ACS_QUERY_MSC_RESPONSE *       haQMSR;
  ACS_QUERY_POL_RESPONSE *       haQPR;
  ACS_QUERY_PRT_RESPONSE *       haQPRR;
  ACS_QUERY_REQ_RESPONSE *       haQRR;
  ACS_QUERY_SCR_RESPONSE *       haQSCRR;
  ACS_QUERY_SRV_RESPONSE *       haQSVR;
  ACS_QUERY_VOL_RESPONSE *       haQVR;
  ACS_SET_CAP_RESPONSE *         haSECR;
  ACS_SET_CLEAN_RESPONSE *       haSECLNR;
  ACS_SET_SCRATCH_RESPONSE *     haSESCRR;
  ACS_START_RESPONSE *           haSR;
  ACS_UNLOCK_DRV_RESPONSE *      haULDR;
  ACS_UNLOCK_VOL_RESPONSE *      haULVR;
  ACS_VARY_ACS_RESPONSE *        haVACSR;
  ACS_VARY_DRV_RESPONSE *        haVDR;
  ACS_VARY_LSM_RESPONSE *        haVLR;
  ACS_VARY_PRT_RESPONSE *        haVPRR;
  ACS_VARY_CAP_RESPONSE *        haVCPR;
  ACS_REGISTER_RESPONSE *        haRR;
  ACS_UNREGISTER_RESPONSE *      haURR;
  ACS_CHECK_REGISTRATION_RESPONSE *      haCRR;
  ACS_DISPLAY_RESPONSE *         haDSP;
  ACS_QU_DRV_GROUP_RESPONSE *    haQDRG;
  ACS_QU_SUBPOOL_NAME_RESPONSE * haQSNR;
  ACS_MOUNT_PINFO_RESPONSE *     haVMR;
  } ACSFMTREC;

typedef union
  {
  ACKNOWLEDGE_RESPONSE *         hACKR;
  AUDIT_RESPONSE *               hAR;
  EJECT_ENTER *                  hAIR;  /* yes, this is screwy -
                                        undocumented shortcut taken ! */
  AUDIT_RESPONSE *               hARP;
  CANCEL_RESPONSE *              hCR;
  CLEAR_LOCK_RESPONSE *          hCLR;
  DEFINE_POOL_RESPONSE *         hDFPR;
  DELETE_POOL_RESPONSE *         hDLPR;
  DISMOUNT_RESPONSE *            hDR;
  EJECT_ENTER *                  hEE;
  IDLE_RESPONSE *                hIR;
  LOCK_RESPONSE *                hLR;
  MOUNT_RESPONSE *               hMR;
  MOUNT_SCRATCH_RESPONSE *       hMSR;
  QUERY_LOCK_RESPONSE *          hQLR;
  QUERY_RESPONSE *               hQR;
  REQUEST_HEADER *               hRH;
  SET_CAP_RESPONSE *             hSECR;
  SET_CLEAN_RESPONSE *           hSECLNR;
  SET_SCRATCH_RESPONSE *         hSESCRR;
  START_RESPONSE *               hSR;
  UNLOCK_RESPONSE *              hULR;
  VARY_RESPONSE *                hVR;
  REGISTER_RESPONSE *            hRR;
  UNREGISTER_RESPONSE *          hURR;
  CHECK_REGISTRATION_RESPONSE *  hCRR;
  DISPLAY_RESPONSE *             hDSP;
  MOUNT_PINFO_RESPONSE *         hVMR;
  }  SVRFMTREC;

#endif

