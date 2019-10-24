#ifndef _ACSAPI_PVT_
#define _ACSAPI_PVT_ 1

/* static char    SccsId[] = "@(#) %full_name: h/incl/acsapi_pvt/2.1.1 %"; */

/**********************************************************************
**   Copyright StorageTek, 1993
**
**   Name:     acsapi.pvt
**
**   Purpose: Repository of data structures and function protoypes used
**            by the ACSAPI Toolkit interface.
**
**
**---------------------------------------------------------------------
**   Maintenance History:
**
**   07/09/93: KJS Created from HSC land (Tom Rethard)
**   08/29/93: KJS Portabilized for BOS/X and MVS with TDR.
**   01/20/94: KJS Removed prototypes for acs_type() and 
**             acs_get_packet_version().
**   01/28/94: KJS Added prototype for acs_get_sockname().
**   01/28/94: KJS Fixed ifndef statement.
**   05/06/94: KJS Changes in support of down level servers.
**   06/23/94: KJS Moved media and drive type defines to apidef.h
**   10/16/01: SLS Added acs_register_int_response prototype.
**
***********************************************************************
*/

/*---------------------------------------------------------------------
**   Required header files
**---------------------------------------------------------------------
*/

#ifndef _ACSAPI_H_
/*#error "acsapi.h" must be #included when using "acsapi.pvt" */
#endif

#ifndef _ACSSYS_PVT_
/*#error "acssys.pvt" must be #included when using "acsapi.pvt" */
#endif

#if(!defined __STDDEF_H)&&(!defined __stddef_h)
#if(!defined _H_STDDEF) && (!defined __size_t)
/*#error <stddef.h> must be included when using "acsapi.pvt"  */
#endif
#endif

/* global var declarations */
extern int              sd_in;   /* module input socket descriptor */
extern TYPE             my_module_type; /* executing module's type */
extern ACCESSID         global_aid;


/* function definitions */
STATUS acs_verify_ssi_running(void);

STATUS acs_select_input(int timeout);

STATUS acs_ipc_read (ALIGNED_BYTES rbuf, size_t * size);

STATUS acs_get_response (ALIGNED_BYTES rbuf, size_t * size);

STATUS acs_ipc_write (ALIGNED_BYTES rbuf, size_t size);

STATUS acs_send_request (ALIGNED_BYTES rbuf, size_t size);

STATUS acs_cvt_v1_v2 (ALIGNED_BYTES rbuf, size_t * byte_count);
STATUS acs_cvt_v2_v3 (ALIGNED_BYTES rbuf, size_t * byte_count);
STATUS acs_cvt_v3_v4 (ALIGNED_BYTES rbuf, size_t * byte_count);
STATUS acs_cvt_v4_v3 (ALIGNED_BYTES rbuf, size_t * byte_count);
STATUS acs_cvt_v3_v2 (ALIGNED_BYTES rbuf, size_t * byte_count);
STATUS acs_cvt_v2_v1 (ALIGNED_BYTES rbuf, size_t * byte_count);

STATUS acs_build_header
  (
  char *         rp,
  size_t         packetLength,
  SEQ_NO         seqNumber,
  COMMAND        requestCommand,
  unsigned char  requestOptions,
  VERSION        packetVersion,
  LOCKID         requestLock
  );

STATUS acs_audit_int_response(char * buffer, ALIGNED_BYTES rbuf);

STATUS acs_audit_fin_response(char * buffer, ALIGNED_BYTES rbuf);

STATUS acs_register_int_response(char * buffer, ALIGNED_BYTES rbuf);

STATUS acs_query_response(char * buffer, ALIGNED_BYTES rbuf);

STATUS acs_vary_response(char * buffer, ALIGNED_BYTES rbuf);

char * acs_get_sockname(void);

#endif
