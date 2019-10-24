#ifndef lint
static char SccId[] = "@(#) %full_name:  1/csrc/t_cdriver.c/13 %";
#endif

/*
 * Copyright (C) 1990,2013, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *      t_cdriver.c
 *
 * Description:
 *      Test driver for the acsapi library.
 *
 * Revision History:
 *      Jim Montgomery  20-Aug-1990.    Original.
 *      Howard Freeman  06-Sep-1991.    Updated for R3.0.
 *      David A. Myers  19-Nov-1991     Changed QU_EXT2_CAP_STATUS to
 *                                                QU_CAP_STATUS
 *      Howard I. Grapek 09-Oct-1992    Added 42 vols to volume list.
 *                                      re-written to conform to R01.00 of the 
 *                                      toolkit.
 *      Howard Freeman  29-Oct-1992.    Updated for R4.0 and general clean-up.
 *      Ken Stickney    09-Jul-1993.    Ansified and Updated for R5.00 of 
 *                                      ACSLS
 *      Ken Stickney    19-Jul-1993     Changed timeout parameter for 
 *                                      acs_response to blocking
 *      Ken Stickney    20-Jul-1993     Included acssys.pvt for mnemonic to 
 *                                      alphanumeric func name translation.
 *      Ken Stickney    20-Sep-1993     Changes in support of query mixed media.
 *      Ken Stickney    23-Sep-1993     Fixed c_query_mount_scratch; 
 *                                      merged ACK RESP printing & checking.
 *      Ken Stickney    11-Nov-1993     Made printing of common data reusable;
 *                                      Established a report mode for use
 *                                      with native ACSLS servers. 
 *      Ken Stickney    14-Jan-1994     Made numerous changes for better 
 *                                      behavior with native ACSLS servers.
 *      Ken Stickney    18-Jan-1994     Added indenting to most of the printfs.
 *      Ken Stickney    20-Jan-1994     Removed private header includes.
 *      Ken Stickney    09-Feb-1994     Changed code to not assume that there
 *                                      is an ACK response for ANY request.
 *      Ken Stickney    09-Feb-1994     Missed some cleanup; now is clean.
 *      Ken Stickney    25-Apr-1994     Changes to handle Toolkit 2.0A 
 *                                      ( which includes down-level server
 *                                        support ).
 *      Ken Stickney    18-May-1994     Changes to st_show_cap_info for 
 *                                      readability of the enter and eject
 *                                      responses. BR#29.
 *      Ken Stickney    16-Jun-1994     Changed force to FALSE for vary lsm
 *                                      & acs.
 *      Ken Stickney    23-Jun-1994     Changes to st_chk_drive_type, 
 *                                      st_show_drive_type, & 
 *                                      st_show_media_type to fix bugs 
 *                                      associated with adding those
 *                                      types to V2 & V3 query drive,
 *                                      query mount, and query mount
 *                                      scratch packets 
 *      Ken Stickney    24-Jun-1994     Created st_show_int_resp_hdr to 
 *                                      fix BR # 35.
 *      Ken Stickney    01-Aug-1994     Added function c_set_access() to
 *                                      exercise access control api.
 *                                      Renamed is_t_acslm to check_mode
 *                                      for semantic reasons.
 *      Ken Stickney    23-Dec-1994     Changes for solaris port.
 *      Ken Stickney    24-Jan-1994     Fix for Toolkit BR#53
 *      Scott Siao      17-Oct-2001     Added support for acs_register,
 *                                      acs_unregister, acs_check_registration.
 *      Scott Siao      12-Nov-2001     Added support for acs_display.
 *      Scott Siao      06-Feb-2002     Added support for acs_mount_pinfo,
 *                                      acs_query_subpool_name.
 *      Scott Siao      12-Feb-2002     Added support for acs_query_drive_group,
 *                                      acs_query_mount_scratch_pinfo.
 *      Scott Siao      01-Mar-2002     Changed VOL_MANUALLY_DELETED to
 *                                      VOL_DELETED.
 *      Anton Vatcky    01-Mar-2002     Removed prototype of getopt for
 *                                      Solaris and AIX.
 *      Scott Siao      23-Apr-2002     Cleaned up code in st_show_lo_vol_status,
 *                                      st_show_acs, c_query_drive_group,
 *                                      mount_scratch, mount_pinfo.
 *      Scott Siao      14-May-2002     Changed v_enter, xeject, register
 *                                      and eject to not be called if using 
 *                                      reporting mode.
 *      Hemendra(Wipro) 30-Dec-2002     Added Linux support. getopt prototype.
 *      Wipro (Subhash) 04-Jun-2004     Modified to display details of
 *                                      mount/dismount events.
 *      Wipro(Hemendra) 17-Jun-2004     Modifications for AIX support
 *                                      Included "time.h" header file
 *                                      Modified st_show_event_drive_status to
 *                                      Calls cl_type for TYPE_* values.
 *                                      Added volume_type_str string array.
 *      Mitch Black     21-Dec-2004     Changes for CDK2.3
 *                                      Added printfs regarding commands that do 
 *                                      not run against a live ACSLS server.
 *                                      Fixed event registration examples.
 *                                      Fixed c_display() example.
 *      Mitch Black     01-Mar-2005     Added in LibStation option.
 *                                      Really fixed c_display().
 *                                      Modified event notification test so it 
 *                                      will work with t_acslm testing.
 *                                      Fixed bug: event_class[20] too small
 *                                      (could cause segmentation violation).
 *      Mitch Black     05-Mar-2005     Added in Real Server option.
 *      Mike Williams   01-Jun-2010     Changed main return type to int. Added
 *                                      include for cl_pub.h Changed vp->acs to
 *                                      vp->acs[i] in printf found in function
 *                                      c_vary_acs.
 *      Joseph Nofi     15-May-2011     XAPI support;
 *                                      Added -x (XAPI) option.
 *                                      Rewrite of most ACSAPI invocation 
 *                                      routines to accept variable parameter lists; 
 *                                      separated check mode routines for "canned"
 *                                      requests from real mode routines for "real"
 *                                      servers; 
 *                                      Added CDR_* output macros to generate traces.                             
 *      Chris Morrison   3-Jan-2013     Add new VOLUME_TYPE enums.
 *
*/


/*********************************************************************/
/* Header Files:                                                     */
/*********************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "acssys.h"
#include "acsapi.h"
#include "cl_pub.h"
#include "identifier.h"
#include "srvcommon.h"


/*********************************************************************/
/* Defines. Typedefs, and Structure Definitions:                     */
/*********************************************************************/
#define TEST_VOL       "SPE007"
#define OVERFLOW       1
#define CDR_MSGLVL_VAR "CDR_MSGLVL"


/*********************************************************************/
/* Global and Static Variable Declarations:                          */
/*********************************************************************/
extern int              optopt;
ALIGNED_BYTES           xbuf[MAX_MESSAGE_SIZE / sizeof (ALIGNED_BYTES)];
ACKNOWLEDGE_RESPONSE   *ap;
ACS_AUDIT_INT_RESPONSE *aip;
ACS_RESPONSE_TYPE       type;
STATUS                  status;
SEQ_NO                  s;
SEQ_NO                  seq_nmbr;
REQ_ID                  req_id;
char                   *acslm_ptr;
int                     lastLockId;
BOOLEAN                 check_mode;
BOOLEAN                 canned_requests;
BOOLEAN                 libstation;
BOOLEAN                 real_server;
BOOLEAN                 xapi;
char                    fin_resp_str[] = "FINAL RESPONSE";
char                    str[80];
int                     p_version;
VOLID                   volumes[MAX_ID] = 
{
    "PB1000", "PB1001", "PB1002", "PB1003", "PB1004", 
    "PB1005", "PB1006", "PB1007", "PB1008", "PB1009", 
    "PB1010", "PB1011", "PB1012", "PB1013", "PB1014", 
    "PB1015", "PB1016", "PB1017", "PB1018", "PB1019", 
    "PB1020", "PB1021", "PB1022", "PB1023", "PB1024", 
    "PB1025", "PB1026", "PB1027", "PB1028", "PB1029", 
    "PB1030", "PB1031", "PB1032", "PB1033", "PB1034", 
    "PB1035", "PB1036", "PB1037", "PB1038", "PB1039", 
    "PB1040", "PB1041"
};
static ALIGNED_BYTES    rbuf[MAX_MESSAGE_SIZE / sizeof(ALIGNED_BYTES)];
char                    str_buffer[80];
char                    st_output_buffer[255];                


/*********************************************************************/
/* Macros:                                                           */
/*********************************************************************/

/*********************************************************************/
/* CDR_MSG and CDR_MEM are for writing output to either STDOUT or    */
/* the trace file, or both.                                          */
/*********************************************************************/
#define CDR_MSG(msgLvL, ...) \
do \
{ \
    char trFuncNamE[]  = SELF; \
    int  trLineNuM     = __LINE__; \
    sprintf(st_output_buffer, __VA_ARGS__); \
    st_output(msgLvL, \
              trFuncNamE, \
              trLineNuM, \
              NULL, \
              0); \
}while(0)


#define CDR_MEM(msgLvL, storageAddresS, storageLengtH, ...) \
do \
{ \
    char trFuncNamE[]  = SELF; \
    int  trLineNuM     = __LINE__; \
    sprintf(st_output_buffer, __VA_ARGS__); \
    st_output(msgLvL, \
              trFuncNamE, \
              trLineNuM, \
              storageAddresS, \
              storageLengtH); \
}while(0)


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
void c_check_mode(void);

void c_real_mode(void);

/*********************************************************************/
/* The ACSAPI invocation routines that issue all of the possible     */
/* ACSAPI requests.                                                  */
/*********************************************************************/
void c_audit_acs(void);

void c_audit_lsm(void);

void c_audit_panel(void);

void c_audit_server(void);

void c_audit_subpanel(void);

void c_cancel(int cancelId);

void c_check_registration(void);

void c_clear_lock_drive(int count, ...);

void c_clear_lock_volume(int count, ...);

void c_define_pool(unsigned long lowWater, 
                   unsigned long highWater, 
                   int           count, ...);

void c_delete_pool(int count, ...);

void c_dismount(int      lockId, 
                char    *volidString, 
                char     driveId[15], 
                BOOLEAN  force);

void c_display(void);

void c_eject(int  lockId, 
             char capId[11], 
             int  count, ...);

void c_enter(char capId[11]);

void c_idle(BOOLEAN force);

void c_lock_drive(int     lockId, 
                  BOOLEAN wait, 
                  int     count, ...);

void c_lock_volume(int     lockId, 
                   BOOLEAN wait,
                   int     count, ...);

void c_mount(int      lockId, 
             char    *volIdString, 
             char     driveId[15], 
             BOOLEAN  readonly, 
             BOOLEAN  bypass);

void c_mount_pinfo(int         lockId, 
                   char       *volIdString, 
                   int         poolId, 
                   char       *mgmtClasString, 
                   MEDIA_TYPE  mediaType, 
                   BOOLEAN     scratch, 
                   BOOLEAN     readonly, 
                   BOOLEAN     bypass, 
                   char       *jobnameString, 
                   char       *stepnameString, 
                   char       *dsnString, 
                   char        driveId[15]);

void c_mount_scratch(int        lockId, 
                     int        poolId, 
                     MEDIA_TYPE mediaType, 
                     char       driveId[15]);

void c_query_acs(int count, ...);

void c_query_cap(int count, ...);

void c_query_clean(void);

void c_query_drive(int count, ...);

void c_query_drive_group(int count, ...);

void c_query_lock_drive(int lockId, 
                        int count, ...);

void c_query_lock_volume(int lockId, 
                         int count, ...);

void c_query_lsm(int count, ...);

void c_query_mm_info(void);

void c_query_mount(int count, ...);

void c_query_mount_scratch(MEDIA_TYPE mediaType, 
                           int        count, ...);

void c_query_mount_scratch_pinfo(MEDIA_TYPE  mediaType, 
                                 char       *mgmtClasString, 
                                 int         count, ...);

void c_query_pool(int count, ...);

void c_query_port(void);

void c_query_request(int count, ...);

void c_query_scratch(int count, ...);

void c_query_server(void);

void c_query_subpool_name(int count, ...);

void c_query_volume(int count, ...);

void c_register(void);

void c_set_access(BOOLEAN set);

void c_set_cap(CAP_PRIORITY cap_priority, 
               CAP_MODE     cap_mode, 
               int          count, ...);

void c_set_clean(BOOLEAN cleanFlag,
                 short   maxUse, 
                 int     lockId, 
                 int     count, ...);

void c_set_scratch(BOOLEAN unscratchFlag, 
                   int     lockId, 
                   int     poolId, 
                   int     count, ...);

void c_start(void);

void c_unlock_drive(int lockId, 
                    int count, ...);

void c_unlock_volume(int lockId, 
                     int count, ...);

void c_unregister(void);

void c_vary_acs(STATE   state,
                BOOLEAN force,
                int     count, ...);

void c_vary_cap(STATE state,
                int   count, ...);

void c_vary_drive(STATE state,
                  int   lockId,
                  int   count, ...);

void c_vary_lsm(STATE   state,
                BOOLEAN force, 
                int     count, ...);

void c_vary_port(STATE state,
                 int   count, ...);

void c_venter(char capId[11],
              int  count, ...);

void c_xeject(int  lockId, 
              char capId[11], 
              int  count, ...);


/*********************************************************************/
/* The print routines that check or output ASCAPI responses.         */
/*********************************************************************/
char *st_reqtostr(int k);

char *st_rttostr(ACS_RESPONSE_TYPE k);

void st_show_ack_res(SEQ_NO        s, 
                     unsigned char mopts, 
                     unsigned char eopts, 
                     LOCKID        lid);

void st_show_audit_int_res(char *callingRoutine);

void st_show_int_resp_hdr(void);

void st_show_final_resp_hdr(void);

void st_show_lo_drv_status(unsigned short     cnt, 
                           ACS_LO_DRV_STATUS *lds_ptr, 
                           DRIVEID           *drive_id,
                           char              *callingRoutine);

void st_show_lo_vol_status(unsigned short     cnt, 
                           ACS_LO_VOL_STATUS *lvs_ptr, 
                           VOLID             *vol_id,
                           char              *callingRoutine);

void st_show_drv_status(unsigned short  i, 
                        QU_DRV_STATUS  *drv_status);

void st_chk_drv_status(unsigned short  i, 
                       QU_DRV_STATUS  *sp, 
                       QU_DRV_STATUS  *sp_expected);

void st_show_drv_info(unsigned short  i, 
                      STATUS         *status, 
                      DRIVEID        *from_server);

void st_chk_drv_info(unsigned short  i, 
                     STATUS         *stat_got, 
                     STATUS          stat_expected, 
                     DRIVEID        *from_server, 
                     DRIVEID        *expected);

void st_show_pool_info(unsigned short  cnt, 
                       STATUS         *s_ptr, 
                       POOL           *p_ptr);

void st_chk_pool_info(unsigned short  cnt, 
                      POOL           *pool_got, 
                      STATUS         *status_got, 
                      POOL           *pool_expected, 
                      STATUS          stat_expected);

void st_show_pool_misc(int           cnt, 
                       unsigned long lwm, 
                       unsigned long hwm, 
                       unsigned long attributes);

void st_chk_pool_misc(unsigned long lwm_got, 
                      unsigned long lwm_exp, 
                      unsigned long hwm_got, 
                      unsigned long hmw_exp, 
                      unsigned long attrs_got, 
                      unsigned long attrs_exp);

void st_show_port_info(unsigned short  i, 
                       PORTID         *pid_got, 
                       STATE           state, 
                       STATUS          status);

void st_chk_port_info(unsigned short  i, 
                      PORTID         *pid_got, 
                      PORTID         *pid_exp, 
                      STATE           state_got, 
                      STATE           state_exp, 
                      STATUS          status_got, 
                      STATUS          status_exp);

void st_show_req_status(unsigned short  i, 
                        QU_REQ_STATUS  *sp);

void st_chk_req_status(unsigned short  i, 
                       QU_REQ_STATUS  *sp_got, 
                       QU_REQ_STATUS  *sp_exp);

void st_show_vol_info(unsigned short  cnt, 
                      STATUS         *status, 
                      VOLID          *from_server);

void st_chk_vol_info(unsigned short  i, 
                     STATUS         *stat_got, 
                     STATUS          stat_expected, 
                     VOLID          *got, 
                     VOLID          *expected);

void st_show_cap_info(int     cnt, 
                      STATUS *stat, 
                      CAPID  *cap_id);

void st_show_cap_requested(CAPID *cap_id);

void st_chk_cap_info(unsigned short  cnt1, 
                     unsigned short  cnt2, 
                     STATUS          *stat_got, 
                     STATUS           stat_exp, 
                     CAPID           *got, 
                     CAPID           *expected);

char *st_cap_mode(CAP_MODE cm);

char *st_cap_priority(CAP_PRIORITY cp);

void st_show_cap_misc(unsigned short  i, 
                      STATE          *cs, 
                      CAP_MODE        cm, 
                      CAP_PRIORITY    cp);

void st_chk_cap_misc(unsigned short  i, 
                     STATE          *cs_got, 
                     STATE           cs_exp, 
                     CAP_MODE        cm_got, 
                     CAP_MODE        cm_exp, 
                     CAP_PRIORITY    cp_got, 
                     CAP_PRIORITY    cp_exp);

void st_show_acs_status(unsigned int   i, 
                        QU_ACS_STATUS *sp);

void st_chk_acs_status(unsigned int            cnt, 
                       ACS_QUERY_ACS_RESPONSE *sp, 
                       ACS                    *acs_id);

void st_show_acs(unsigned int i, 
                 STATUS       stat_got, 
                 ACS          acs_got);

void st_chk_acs(unsigned int cnt, 
                STATUS       stat_got, 
                STATUS       stat_exp, 
                ACS          acs_got, 
                ACS          acs_exp);

void st_show_usage(QU_CLN_STATUS *cstat);

void st_show_cellid(unsigned short  i, 
                    STATUS         *stat, 
                    CELLID         *from_server);

void st_show_media_info(unsigned short        i, 
                        QU_MEDIA_TYPE_STATUS *mip);

void st_chk_media_info(unsigned short        i, 
                       QU_MEDIA_TYPE_STATUS *mip, 
                       QU_MEDIA_TYPE_STATUS *mp);

void st_show_media_type(unsigned short i, 
                        MEDIA_TYPE     mtype);

void st_chk_cellid(unsigned short  i, 
                   CELLID         *got, 
                   CELLID         *exp);

void st_show_lsm_id(unsigned short  i, 
                    STATUS         *stat, 
                    LSMID          *lsm_id);

void st_chk_lsm_id(unsigned short  i, 
                   STATUS         *stat_got, 
                   STATUS          stat_expected, 
                   LSMID          *got, 
                   LSMID          *exptd);

void st_chk_lock_info(unsigned short i, 
                      LOCKID         lock_got, 
                      LOCKID         lock_exp, 
                      unsigned long  ld_got, 
                      unsigned long  ld_exp, 
                      unsigned long  lp_got, 
                      unsigned long  lp_exp, 
                      USERID        *ui_got, 
                      USERID        *ui_exp);

void st_show_lock_info(unsigned short i, 
                       LOCKID         lock_id, 
                       unsigned long  lock_dur, 
                       unsigned long  locks_pend, 
                       USERID        *user);

void st_chk_media_type(unsigned short i, 
                       MEDIA_TYPE     got, 
                       MEDIA_TYPE     expected);

void st_chk_usage(unsigned short  i, 
                  QU_CLN_STATUS  *got, 
                  QU_CLN_STATUS  *expected);

void st_show_panel_info(unsigned short  i, 
                        STATUS         *stat, 
                        PANELID        *from_server);

void st_chk_panel_info(unsigned short  i, 
                       STATUS         *stat_got, 
                       STATUS          stat_exp, 
                       PANELID        *from_server, 
                       PANELID        *expected);

void st_show_state(unsigned short i, 
                   STATE          state);

void st_chk_state(unsigned short i, 
                  STATE          state_got, 
                  STATE          state_expected);

void st_chk_drive_type(unsigned short i, 
                       DRIVE_TYPE     got, 
                       DRIVE_TYPE     expected);

void st_show_drive_type_info(unsigned short        i, 
                             QU_DRIVE_TYPE_STATUS *dip);

void st_chk_drive_type_info(unsigned short        i, 
                            QU_DRIVE_TYPE_STATUS *dip, 
                            QU_DRIVE_TYPE_STATUS *dp);

void st_show_drive_type(unsigned short i, 
                        DRIVE_TYPE dtype);

void st_show_freecells(unsigned short i, 
                       FREECELLS      fcells);

void st_chk_freecells(unsigned short i, 
                      FREECELLS      got, 
                      FREECELLS      expected);

void st_show_requests(unsigned short  i, 
                      REQ_SUMMARY    *reqs);

void st_chk_requests(REQ_SUMMARY *got, 
                     REQ_SUMMARY *expected);

void st_show_register_info(ACS_REGISTER_RESPONSE *from_server);

void st_show_register_status(STATUS status);

void st_show_event_resource_status(ACS_REGISTER_RESPONSE *from_server);

void st_show_event_register_status(EVENT_REGISTER_STATUS *from_server);

void st_show_display_info(ACS_DISPLAY_RESPONSE *from_server);

void st_show_event_drive_status(ACS_REGISTER_RESPONSE *from_server);

void st_output(int   messageLevel,
               char *functionName,
               int   lineNumber,
               char *storageAddress,
               int   storageLength);

BOOLEAN no_variable_part();


/*********************************************************************/
/* Utility routines used by the print functions.                     */
/*********************************************************************/
char *decode_mopts(unsigned char msgopt);

char *decode_vers(long vers);

char *decode_eopts(unsigned char extopt);


#ifdef SOLARIS
#elif AIX
#elif LINUX
#else
int getopt(int, char**, char*);
#endif


#undef SELF
#define SELF "t_cdriver"
int main(int    argc, 
         char **argv)
{
    int             c;

    GLOBAL_SRVCOMMON_SET("CLIENT");

    canned_requests = FALSE;
    check_mode = FALSE;
    libstation = FALSE;
    real_server = FALSE;
    xapi = FALSE;

    /*****************************************************************/
    /* Check the command line arguments.                             */
    /*****************************************************************/
    while ((c = getopt(argc, argv, "clrx")) != - 1)
    {
        switch (c)
        {
        case 'c':
            check_mode = TRUE;
            canned_requests = TRUE;
            break;
        case 'l':
            libstation = TRUE;
            break;
        case 'r':
            real_server = TRUE;
            break;
        case 'x':
            xapi = TRUE;
            break;
        default:   
            CDR_MSG(0, "\n%s: Invalid option %c. \n\tValid options are:\n", SELF, optopt);  
            CDR_MSG(0, "\t\tc = Check Mode (check returned values against hard-coded stub values)\n");
            CDR_MSG(0, "\t\tl = Library Station (tests running against LibStation server)\n");
            CDR_MSG(0, "\t\tr = Real Server (tests running against a real server)\n\n");
            CDR_MSG(0, "\t\tx = XAPI Server (tests running with XAPI conversion enabled)\n\n");
            exit(2);
        }
    }

    /*****************************************************************/
    /* If -c (check mode) specified with -r or -x,                   */
    /* then turn off check mode (but still do the "canned" requests).*/
    /*****************************************************************/
    if ((real_server) &&
        (check_mode))
    {
        CDR_MSG(4, "\n%s: -c specified with -r: internal check mode disabled\n", SELF);  
        check_mode = FALSE;
    }

    if ((xapi) &&
        (check_mode))
    {
        CDR_MSG(4, "\n%s: -c specified with -x: internal check mode disabled\n", SELF);  
        check_mode = FALSE;
    }

    p_version = acs_get_packet_version();

    CDR_MSG(0, "Starting the ACSLS API test driver with version %d packets\n",
            p_version);

    /*****************************************************************/
    /* If -c (check mode), or "canned" request testing indicated,    */
    /* then do the "canned" tests.                                   */
    /*****************************************************************/
    if ((check_mode) ||
        (canned_requests))
    {
        c_check_mode();
    }
    /*****************************************************************/
    /* Otherwise, if NOT -c (check mode) or "canned" request testing */
    /* then do the real server or "tailored" request tests.          */
    /*****************************************************************/
    else
    {
        c_real_mode();
    }

    CDR_MSG(0, "\nCompleting the ACSLS API test driver\n");

    exit(0);
}


#undef SELF
#define SELF "c_check_mode"
/*********************************************************************/
/* This function duplicates the old canned requests for non-real     */
/* server, check mode (selected by "-c" option).                     */
/*                                                                   */
/* These are the "canned" ACSAPI and XAPI requests:                  */
/* o  Canned ACSAPI requests have corresponding hard coded           */
/*    response checking routines.                                    */
/* o  Canned XAPI requests have a corresponding distributed          */
/*    trace_decode file used to check responses.                     */
/*                                                                   */
/* Do NOT change any of the parameter lists in c_check_mode.         */
/*                                                                   */
/* The parameter lists in the c_real_mode function may be            */
/* customized for a specific real ACSLS, LibStation, or HTTP server. */
/*********************************************************************/
void c_check_mode(void)
{
    /*****************************************************************/
    /* ACSAPI (i.e. non-XAPI) "canned" requests.                     */
    /*****************************************************************/
    if (!(xapi))
    {
        CDR_MSG(0, "\n%s: Testing set_access\n", SELF);
        c_set_access(TRUE);
        CDR_MSG(0, "\n%s: Testing query_server\n", SELF);
        c_query_server();
        CDR_MSG(0, "\n%s: Testing query_acs\n", SELF);
        c_query_acs(1, "000");
        CDR_MSG(0, "\n%s: Testing query_cap\n", SELF);
        c_query_cap(1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing query_clean\n", SELF);
        c_query_clean();
        CDR_MSG(0, "\n%s: Testing query_drive\n", SELF);
        c_query_drive(1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing query_lsm\n", SELF);
        c_query_lsm(1, "000:000");
        CDR_MSG(0, "\n%s: Testing query_mm_info\n", SELF);
        c_query_mm_info(); 
        CDR_MSG(0, "\n%s: Testing query_mount\n", SELF);
        c_query_mount(1, TEST_VOL);
        CDR_MSG(0, "\n%s: Testing query_mount_scratch\n", SELF);
        c_query_mount_scratch(ANY_MEDIA_TYPE, 1, 0);
        CDR_MSG(0, "\n%s: Testing query_pool\n", SELF);
        c_query_pool(1, 0);
        CDR_MSG(0, "\n%s: Testing query_port\n", SELF);
        c_query_port();
        CDR_MSG(0, "\n%s: Testing query_request\n", SELF);
        c_query_request(1, 0);
        CDR_MSG(0, "\n%s: Testing query_scratch\n", SELF);
        c_query_scratch(1, 0);
        CDR_MSG(0, "\n%s: Testing query_volume\n", SELF);
        c_query_volume(1, "PB1000");
        CDR_MSG(0, "\n%s: Testing query_lock_drive\n", SELF);
        c_query_lock_drive(NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing query_lock_volume\n", SELF);
        c_query_lock_volume(NO_LOCK_ID, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing audit_server\n", SELF);
        c_audit_server();
        CDR_MSG(0, "\n%s: Testing audit_acs\n", SELF);
        c_audit_acs();
        CDR_MSG(0, "\n%s: Testing audit_lsm\n", SELF);
        c_audit_lsm();
        CDR_MSG(0, "\n%s: Testing audit_panel\n", SELF);
        c_audit_panel();
        CDR_MSG(0, "\n%s: Testing audit_subpanel\n", SELF);
        c_audit_subpanel();
        CDR_MSG(0, "\n%s: Testing clear_lock_drive\n", SELF);
        c_clear_lock_drive(1, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing clear_lock_volume\n", SELF);
        c_clear_lock_volume(1, "PB0300");
        CDR_MSG(0, "\n%s: Testing cancel\n", SELF);
        c_cancel(201);
        CDR_MSG(0, "\n%s: Testing define_pool\n", SELF);
        c_define_pool(5, 9, 2, 43, 44);
        CDR_MSG(0, "\n%s: Testing delete_pool\n", SELF);
        c_delete_pool(2, 5, 6);
        CDR_MSG(0, "\n%s: Testing dismount(force)\n", SELF);
        c_dismount(10, "PB1000", "000:000:000:000", TRUE);
        CDR_MSG(0, "\n%s: Testing eject\n", SELF);
        c_eject(44, "001:002:000", 42, 
                "PB1000", "PB1001", "PB1002", "PB1003", "PB1004", 
                "PB1005", "PB1006", "PB1007", "PB1008", "PB1009", 
                "PB1010", "PB1011", "PB1012", "PB1013", "PB1014", 
                "PB1015", "PB1016", "PB1017", "PB1018", "PB1019", 
                "PB1020", "PB1021", "PB1022", "PB1023", "PB1024", 
                "PB1025", "PB1026", "PB1027", "PB1028", "PB1029", 
                "PB1030", "PB1031", "PB1032", "PB1033", "PB1034", 
                "PB1035", "PB1036", "PB1037", "PB1038", "PB1039", 
                "PB1040", "PB1041");
        CDR_MSG(0, "\n%s: Testing enter\n", SELF);
        c_enter("000:000:000");
        CDR_MSG(0, "\n%s: Testing idle\n", SELF);
        c_idle(FALSE);
        CDR_MSG(0, "\n%s: Testing start\n", SELF);
        c_start();
        CDR_MSG(0, "\n%s: Testing idle(force)\n", SELF);
        c_idle(TRUE);
        CDR_MSG(0, "\n%s: Testing start\n", SELF);
        c_start();
        CDR_MSG(0, "\n%s: Testing lock_drive\n", SELF);
        c_lock_drive(NO_LOCK_ID, TRUE, 1, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing lock_volume\n", SELF);
        c_lock_drive(NO_LOCK_ID, TRUE, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing mount\n", SELF);
        c_mount(555, "PB1000", "001:002:010:003", TRUE, TRUE);
        CDR_MSG(0, "\n%s: Testing set_accessid\n", SELF);
        CDR_MSG(0, "\t%s: Clearing the user id\n", SELF);
        c_set_access(FALSE);
        CDR_MSG(0, "\n%s: Testing mount_scratch\n", SELF);
        c_mount_scratch(NO_LOCK_ID, 5, ANY_MEDIA_TYPE, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing set_cap\n", SELF);
        c_set_cap(5, CAP_MODE_SAME, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing set_clean\n", SELF);
        c_set_clean(TRUE, 700, NO_LOCK_ID, 1, "PB0300-PB0302");
        CDR_MSG(0, "\n%s: Testing set_scratch\n", SELF);
        c_set_scratch(TRUE, 88, 7, 1, "PB0300-PB0302");
        CDR_MSG(0, "\n%s: Testing unlock_drive\n", SELF);
        c_unlock_drive(NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing unlock_volume\n", SELF);
        c_unlock_volume(NO_LOCK_ID, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing vary_drive\n", SELF);
        c_vary_drive(STATE_OFFLINE, NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_drive\n", SELF);
        c_vary_drive(STATE_ONLINE, NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_port\n", SELF);
        c_vary_port(STATE_OFFLINE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_port\n", SELF);
        c_vary_port(STATE_ONLINE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_cap\n", SELF);
        c_vary_cap(STATE_OFFLINE, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_cap\n", SELF);
        c_vary_cap(STATE_ONLINE, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_lsm\n", SELF);
        c_vary_lsm(STATE_OFFLINE, TRUE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_lsm\n", SELF);
        c_vary_lsm(STATE_ONLINE, FALSE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_acs\n", SELF);
        c_vary_acs(STATE_OFFLINE, TRUE, 1, "000");
        CDR_MSG(0, "\n%s: Testing vary_acs\n", SELF);
        c_vary_acs(STATE_ONLINE, FALSE, 1, "000");
        CDR_MSG(0, "\n%s: Testing venter\n", SELF);
        c_venter("000:000:000", 21, 
                 "PB1000", "PB1001", "PB1002", "PB1003", "PB1004", 
                 "PB1005", "PB1006", "PB1007", "PB1008", "PB1009", 
                 "PB1010", "PB1011", "PB1012", "PB1013", "PB1014", 
                 "PB1015", "PB1016", "PB1017", "PB1018", "PB1019", 
                 "PB1020");
        CDR_MSG(0, "\n%s: Testing xeject\n", SELF);
        c_xeject(NO_LOCK_ID, "000:000:000", 1, "PB0300-PB0302");  
        CDR_MSG(0, "\n%s: Testing event notification (register, check registration, unregister)\n", SELF);
        c_register();
        CDR_MSG(0, "\n%s: Testing display\n", SELF);
        c_display();

        if (libstation)
        {
            CDR_MSG(0, "\n%s: Testing mount_pinfo\n", SELF);
            c_mount_pinfo(999, "VV0499", 5, "MGMTCLS1", 
                          -2, FALSE, FALSE, FALSE,
                          "JOB00001", "STEP001", "DATA01", "058:003:004:000");
            CDR_MSG(0, "\n%s: Testing query_subpool_name\n", SELF);
            c_query_subpool_name(20, 
                                 "SUBPOOLNAM0",  "SUBPOOLNAM1",  "SUBPOOLNAM2",  "SUBPOOLNAM3",  
                                 "SUBPOOLNAM4",  "SUBPOOLNAM5",  "SUBPOOLNAM6",  "SUBPOOLNAM7",  
                                 "SUBPOOLNAM8",  "SUBPOOLNAM9",  "SUBPOOLNAM10", "SUBPOOLNAM11",  
                                 "SUBPOOLNAM12", "SUBPOOLNAM13", "SUBPOOLNAM14", "SUBPOOLNAM15", 
                                 "SUBPOOLNAM16", "SUBPOOLNAM17", "SUBPOOLNAM18", "SUBPOOLNAM19");
            CDR_MSG(0, "\n%s: Testing query_drive_group\n", SELF);
            c_query_drive_group(19, 
                                "GROUP00",  "GROUP01",  "GROUP02",  "GROUP03",  
                                "GROUP04",  "GROUP05",  "GROUP06",  "GROUP07",  
                                "GROUP08",  "GROUP09",  "GROUP010", "GROUP011",  
                                "GROUP012", "GROUP013", "GROUP014", "GROUP015", 
                                "GROUP016", "GROUP017", "GROUP018");
            CDR_MSG(0, "\n%s: Testing query_mount_scratch_pinfo\n", SELF);
            c_query_mount_scratch_pinfo(ANY_MEDIA_TYPE, "MGMTCLS1", 1, 18);
        }
    }
    /*****************************************************************/
    /* XAPI "canned" requests.                                       */
    /*****************************************************************/
    else
    {
        CDR_MSG(0, "\nTesting set_access(TRUE)\n");
        c_set_access(TRUE);  
        CDR_MSG(0, "\nTesting idle(FALSE)\n");
        c_idle(FALSE);
        CDR_MSG(0, "\nTesting start()\n");
        c_start();
        CDR_MSG(0, "\nTesting register()\n");
        c_register();
        CDR_MSG(0, "\nTesting query_server()\n");
        c_query_server();
        CDR_MSG(0, "\nTesting check_registration()\n");
        c_check_registration();  
        CDR_MSG(0, "\nTesting query_port()\n");
        c_query_port();
        CDR_MSG(0, "\nTesting define_pool(10, 100, 3, 1, 2, 200)\n");
        c_define_pool(10, 100, 3, 1, 2, 200);
        CDR_MSG(0, "\nTesting delete_pool(3, 1, 2, 200)\n");
        c_delete_pool(3, 1, 2, 200);
        CDR_MSG(0, "\nTesting query_mm_info()\n");
        c_query_mm_info(); 
        CDR_MSG(0, "\nTesting query_acs(0)\n");
        c_query_acs(0);
        CDR_MSG(0, "\nTesting query_acs(1, \"005\")\n");
        c_query_acs(1, "005");
        CDR_MSG(0, "\nTesting query_lsm(0)\n");
        c_query_lsm(0);
        CDR_MSG(0, "\nTesting query_drive_group(0)\n");
        c_query_drive_group(0);
        CDR_MSG(0, "\nTesting query_drive_group(2, \"DVTSS16\", \"DVTSS17\")\n");
        c_query_drive_group(2, "DVTSS16", "DVTSS17");
        CDR_MSG(0, "\nTesting query_cap(0)\n");
        c_query_cap(0);
        CDR_MSG(0, "\nTesting query_drive(0)\n");
        c_query_drive(0);
        CDR_MSG(0, "\nTesting query_subpool_name(0)\n");
        c_query_subpool_name(0);
        CDR_MSG(0, "\nTesting query_subpool_name(3, \"REAL1\", \"REAL2\", \"REAL3\")\n");
        c_query_subpool_name(3, "REAL1", "REAL2", "REAL3");
        CDR_MSG(0, "\nTesting query_pool(0)\n");
        c_query_pool(0);
        CDR_MSG(0, "\nTesting query_pool(3, 1, 200, 5)\n");
        c_query_pool(3, 1, 200, 5);
        CDR_MSG(0, "\nTesting query_scratch(0)\n");
        c_query_scratch(0);
        CDR_MSG(0, "\nTesting query_scratch(3, 1, 200, 5)\n");
        c_query_scratch(3, 1, 200, 5);
        CDR_MSG(0, "\nTesting query_clean()\n");
        c_query_clean();
        CDR_MSG(0, "\nTesting query_volume(0)\n");
        c_query_volume(0);
        CDR_MSG(0, "\nTesting query_volume(1, \"NOTFND\")\n");
        c_query_volume(1, "NOTFND");
        CDR_MSG(0, "\nTesting query_volume(1, \"DRL665\")\n");
        c_query_volume(1, "DRL665");
        CDR_MSG(0, "\nTesting query_volume(1, \"DX0010\")\n");
        c_query_volume(1, "DX0010");
        CDR_MSG(0, "\nTesting set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
        c_set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
        c_set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting set_clean(TRUE, 10, NO_LOCK_ID, 1, \"DRL001-DRL099\")\n");
        c_set_clean(TRUE, 10, NO_LOCK_ID, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting query_mount(1, \"DRL970\")\n");
        c_query_mount(1, "DRL970");
        CDR_MSG(0, "\nTesting query_mount(1, \"NOTFND\")\n");
        c_query_mount(1, "NOTFND");
        CDR_MSG(0, "\nTesting query_mount(1, \"DX0010\")\n");
        c_query_mount(1, "DX0010");
        CDR_MSG(0, "\nTesting query_mount_scratch(ANY_MEDIA_TYPE, 1, 10)\n");
        c_query_mount_scratch(ANY_MEDIA_TYPE, 1, 10);
        CDR_MSG(0, "\nTesting query_mount_scratch_pinfo(ANY_MEDIA_TYPE, \"MGMT1\", 1, 10)\n");
        c_query_mount_scratch_pinfo(ANY_MEDIA_TYPE, "MGMT1", 1, 10);
        CDR_MSG(0, "\nTesting audit server()\n");
        c_audit_server();
        CDR_MSG(0, "\nTesting audit acs()\n");
        c_audit_acs();
        CDR_MSG(0, "\nTesting audit lsm()\n");
        c_audit_lsm();
        CDR_MSG(0, "\nTesting audit panel()\n");
        c_audit_panel();
        CDR_MSG(0, "\nTesting audit subpanel()\n");
        c_audit_subpanel();
        CDR_MSG(0, "\nTesting cancel(201)\n");
        c_cancel(201);
        CDR_MSG(0, "\nTesting mount(0, \"DRL970\", \"000:001:009:006\", TRUE, TRUE)\n");
        c_mount(0, "DRL970", "000:001:009:006", TRUE, TRUE);
        CDR_MSG(0, "\nTesting dismount(0, \"DRL970\", \"000:001:009:006\", TRUE)\n");
        c_dismount(0, "DRL970", "000:001:009:006", TRUE);
        CDR_MSG(0, "\nTesting query_request(0)\n");
        c_query_request(0);
        CDR_MSG(0, "\nTesting display()\n");
        c_display();  
        CDR_MSG(0, "\nTesting eject(0, \"000:000:000\", 3, \"DRL001\", \"DRL002\", \"DRL003\")\n");
        c_eject(0, "000:000:000", 3, "DRL001", "DRL002", "DRL003");  
        CDR_MSG(0, "\nTesting xeject(0, \"000:000:000\", 2, \"DRL800-DRL810\", \"DRL900-DRL910\"\n");
        c_xeject(0, "000:000:000", 2, "DRL800-DRL810", "DRL900-DRL910");  
        CDR_MSG(0, "\nTesting enter(\"000:000:000\")\n");
        c_enter("000:000:000");  
        CDR_MSG(0, "\nTesting venter(\"000:000:000\"), 2, \"V00001\", \"V00002\"\n");
        c_venter("000:000:000", 2, "V00001", "V00002");  
        CDR_MSG(0, "\nTesting vary_acs(STATE_OFFLINE, TRUE, 1, \"000\")\n");
        c_vary_acs(STATE_OFFLINE, TRUE, 1, "000");
        CDR_MSG(0, "\nTesting vary_acs(STATE_ONLINE, FALSE, 1, \"000\")\n");
        c_vary_acs(STATE_ONLINE, FALSE, 1, "000");
        CDR_MSG(0, "\nTesting vary_cap(STATE_OFFLINE, 1, \"000:000:000\")\n");
        c_vary_cap(STATE_OFFLINE, 1, "000:000:000");
        CDR_MSG(0, "\nTesting vary_cap(STATE_ONLINE, 1, \"000:000:000\")\n");
        c_vary_cap(STATE_ONLINE, 1, "000:000:000");
        CDR_MSG(0, "\nTesting vary_drive(STATE_OFFLINE, 0, 1, \"001:000:010:000\")\n");
        c_vary_drive(STATE_OFFLINE, 0, 1, "001:000:010:000");
        CDR_MSG(0, "\nTesting vary_drive(STATE_ONLINE, 0, 1, \"001:000:010:000\")\n");
        c_vary_drive(STATE_ONLINE, 0, 1, "001:000:010:000");
        CDR_MSG(0, "\nTesting vary_lsm(STATE_OFFLINE, TRUE, 1, \"000:000\")\n");
        c_vary_lsm(STATE_OFFLINE, TRUE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_lsm(STATE_ONLINE, FALSE, 1, \"000:000\")\n");
        c_vary_lsm(STATE_ONLINE, FALSE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_port(STATE_OFFLINE, 1, \"000:000\")\n");
        c_vary_port(STATE_OFFLINE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_port(STATE_ONLINE, 1, \"000:000\")\n");
        c_vary_port(STATE_ONLINE, 1, "000:000");
        CDR_MSG(0, "\nTesting set_access(FALSE)\n");
        c_set_access(FALSE);  
    }

    return;
}

#undef SELF
#define SELF "c_real_mode"
/*********************************************************************/
/* This following test function calls may be tailored to a specific  */
/* real server environment (selected by not specifying "-c" option). */
/*********************************************************************/
void c_real_mode(void)
{
    char            reply[255];

    /*****************************************************************/
    /* The following duplicate the c_check_mode ACSAPI and XAPI      */
    /* calls, they may be commented out and/or modified for your     */
    /* real server.                                                  */
    /*****************************************************************/

    if (!(xapi))
    {
        CDR_MSG(0, "\n%s: Testing set_access\n", SELF);
        c_set_access(TRUE);
        CDR_MSG(0, "\n%s: Testing query_server\n", SELF);
        c_query_server();
        CDR_MSG(0, "\n%s: Testing query_acs\n", SELF);
        c_query_acs(1, "000");
        CDR_MSG(0, "\n%s: Testing query_cap\n", SELF);
        c_query_cap(1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing query_clean\n", SELF);
        c_query_clean();
        CDR_MSG(0, "\n%s: Testing query_drive\n", SELF);
        c_query_drive(1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing query_lsm\n", SELF);
        c_query_lsm(1, "000:000");
        CDR_MSG(0, "\n%s: Testing query_mm_info\n", SELF);
        c_query_mm_info(); 
        CDR_MSG(0, "\n%s: Testing query_mount\n", SELF);
        c_query_mount(1, TEST_VOL);
        CDR_MSG(0, "\n%s: Testing query_mount_scratch\n", SELF);
        c_query_mount_scratch(ANY_MEDIA_TYPE, 1, 0);
        CDR_MSG(0, "\n%s: Testing query_pool\n", SELF);
        c_query_pool(1, 0);
        CDR_MSG(0, "\n%s: Testing query_port\n", SELF);
        c_query_port();
        CDR_MSG(0, "\n%s: Testing query_request\n", SELF);
        c_query_request(1, 0);
        CDR_MSG(0, "\n%s: Testing query_scratch\n", SELF);
        c_query_scratch(1, 0);
        CDR_MSG(0, "\n%s: Testing query_volume\n", SELF);
        c_query_volume(1, "PB1000");
        CDR_MSG(0, "\n%s: Testing query_lock_drive\n", SELF);
        c_query_lock_drive(NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing query_lock_volume\n", SELF);
        c_query_lock_volume(NO_LOCK_ID, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing audit_server\n", SELF);
        c_audit_server();
        CDR_MSG(0, "\n%s: Testing audit_acs\n", SELF);
        c_audit_acs();
        CDR_MSG(0, "\n%s: Testing audit_lsm\n", SELF);
        c_audit_lsm();
        CDR_MSG(0, "\n%s: Testing audit_panel\n", SELF);
        c_audit_panel();
        CDR_MSG(0, "\n%s: Testing audit_subpanel\n", SELF);
        c_audit_subpanel();
        CDR_MSG(0, "\n%s: Testing clear_lock_drive\n", SELF);
        c_clear_lock_drive(1, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing clear_lock_volume\n", SELF);
        c_clear_lock_volume(1, "PB0300");
        CDR_MSG(0, "\n%s: Testing cancel\n", SELF);
        c_cancel(201);
        CDR_MSG(0, "\n%s: Testing define_pool\n", SELF);
        c_define_pool(5, 9, 2, 43, 44);
        CDR_MSG(0, "\n%s: Testing delete_pool\n", SELF);
        c_delete_pool(2, 5, 6);
        CDR_MSG(0, "\n%s: Testing dismount(force)\n", SELF);
        c_dismount(10, "PB1000", "000:000:000:000", TRUE);
        CDR_MSG(0, "\n%s: Testing eject\n", SELF);
        c_eject(44, "001:002:000", 42, 
                "PB1000", "PB1001", "PB1002", "PB1003", "PB1004", 
                "PB1005", "PB1006", "PB1007", "PB1008", "PB1009", 
                "PB1010", "PB1011", "PB1012", "PB1013", "PB1014", 
                "PB1015", "PB1016", "PB1017", "PB1018", "PB1019", 
                "PB1020", "PB1021", "PB1022", "PB1023", "PB1024", 
                "PB1025", "PB1026", "PB1027", "PB1028", "PB1029", 
                "PB1030", "PB1031", "PB1032", "PB1033", "PB1034", 
                "PB1035", "PB1036", "PB1037", "PB1038", "PB1039", 
                "PB1040", "PB1041");
        CDR_MSG(0, "\n%s: Testing enter\n", SELF);
        c_enter("000:000:000");
        CDR_MSG(0, "\n%s: Testing idle\n", SELF);
        c_idle(FALSE);
        CDR_MSG(0, "\n%s: Testing start\n", SELF);
        c_start();
        CDR_MSG(0, "\n%s: Testing idle(force)\n", SELF);
        c_idle(TRUE);
        CDR_MSG(0, "\n%s: Testing start\n", SELF);
        c_start();
        CDR_MSG(0, "\n%s: Testing lock_drive\n", SELF);
        c_lock_drive(NO_LOCK_ID, TRUE, 1, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing lock_volume\n", SELF);
        c_lock_drive(NO_LOCK_ID, TRUE, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing mount\n", SELF);
        c_mount(555, "PB1000", "001:002:010:003", TRUE, TRUE);
        CDR_MSG(0, "\n%s: Testing set_accessid\n", SELF);
        CDR_MSG(0, "\t%s: Clearing the user id\n", SELF);
        c_set_access(FALSE);
        CDR_MSG(0, "\n%s: Testing mount_scratch\n", SELF);
        c_mount_scratch(NO_LOCK_ID, 5, ANY_MEDIA_TYPE, "001:002:010:003");
        CDR_MSG(0, "\n%s: Testing set_cap\n", SELF);
        c_set_cap(5, CAP_MODE_SAME, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing set_clean\n", SELF);
        c_set_clean(TRUE, 700, NO_LOCK_ID, 1, "PB0300-PB0302");
        CDR_MSG(0, "\n%s: Testing set_scratch\n", SELF);
        c_set_scratch(TRUE, 88, 7, 1, "PB0300-PB0302");
        CDR_MSG(0, "\n%s: Testing unlock_drive\n", SELF);
        c_unlock_drive(NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing unlock_volume\n", SELF);
        c_unlock_volume(NO_LOCK_ID, 1, "PB1000");
        CDR_MSG(0, "\n%s: Testing vary_drive\n", SELF);
        c_vary_drive(STATE_OFFLINE, NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_drive\n", SELF);
        c_vary_drive(STATE_ONLINE, NO_LOCK_ID, 1, "000:000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_port\n", SELF);
        c_vary_port(STATE_OFFLINE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_port\n", SELF);
        c_vary_port(STATE_ONLINE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_cap\n", SELF);
        c_vary_cap(STATE_OFFLINE, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_cap\n", SELF);
        c_vary_cap(STATE_ONLINE, 1, "000:000:000");
        CDR_MSG(0, "\n%s: Testing vary_lsm\n", SELF);
        c_vary_lsm(STATE_OFFLINE, TRUE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_lsm\n", SELF);
        c_vary_lsm(STATE_ONLINE, FALSE, 1, "000:000");
        CDR_MSG(0, "\n%s: Testing vary_acs\n", SELF);
        c_vary_acs(STATE_OFFLINE, TRUE, 1, "000");
        CDR_MSG(0, "\n%s: Testing vary_acs\n", SELF);
        c_vary_acs(STATE_ONLINE, FALSE, 1, "000");
        CDR_MSG(0, "\n%s: Testing venter\n", SELF);
        c_venter("000:000:000", 21, 
                 "PB1000", "PB1001", "PB1002", "PB1003", "PB1004", 
                 "PB1005", "PB1006", "PB1007", "PB1008", "PB1009", 
                 "PB1010", "PB1011", "PB1012", "PB1013", "PB1014", 
                 "PB1015", "PB1016", "PB1017", "PB1018", "PB1019", 
                 "PB1020");
        CDR_MSG(0, "\n%s: Testing xeject\n", SELF);
        c_xeject(NO_LOCK_ID, "000:000:000", 1, "PB0300-PB0302");  
        CDR_MSG(0, "\n%s: Testing event notification (register, check registration, unregister)\n", SELF);
        c_register();
        CDR_MSG(0, "\n%s: Testing display\n", SELF);
        c_display();

        if (libstation)
        {
            CDR_MSG(0, "\n%s: Testing mount_pinfo\n", SELF);
            c_mount_pinfo(999, "VV0499", 5, "MGMTCLS1", 
                          -2, FALSE, FALSE, FALSE,
                          "JOB00001", "STEP001", "DATA01", "058:003:004:000");
            CDR_MSG(0, "\n%s: Testing query_subpool_name\n", SELF);
            c_query_subpool_name(20, 
                                 "SUBPOOLNAM0",  "SUBPOOLNAM1",  "SUBPOOLNAM2",  "SUBPOOLNAM3",  
                                 "SUBPOOLNAM4",  "SUBPOOLNAM5",  "SUBPOOLNAM6",  "SUBPOOLNAM7",  
                                 "SUBPOOLNAM8",  "SUBPOOLNAM9",  "SUBPOOLNAM10", "SUBPOOLNAM11",  
                                 "SUBPOOLNAM12", "SUBPOOLNAM13", "SUBPOOLNAM14", "SUBPOOLNAM15", 
                                 "SUBPOOLNAM16", "SUBPOOLNAM17", "SUBPOOLNAM18", "SUBPOOLNAM19");
            CDR_MSG(0, "\n%s: Testing query_drive_group\n", SELF);
            c_query_drive_group(19, 
                                "GROUP00",  "GROUP01",  "GROUP02",  "GROUP03",  
                                "GROUP04",  "GROUP05",  "GROUP06",  "GROUP07",  
                                "GROUP08",  "GROUP09",  "GROUP010", "GROUP011",  
                                "GROUP012", "GROUP013", "GROUP014", "GROUP015", 
                                "GROUP016", "GROUP017", "GROUP018");
            CDR_MSG(0, "\n%s: Testing query_mount_scratch_pinfo\n", SELF);
            c_query_mount_scratch_pinfo(ANY_MEDIA_TYPE, "MGMTCLS1", 1, 18);
        }
    }
    else
    {
        CDR_MSG(0, "\nTesting set_access(TRUE)\n");
        c_set_access(TRUE);  
        CDR_MSG(0, "\nTesting idle(FALSE)\n");
        c_idle(FALSE);
        CDR_MSG(0, "\nTesting start()\n");
        c_start();
        CDR_MSG(0, "\nTesting register()\n");
        c_register();
        CDR_MSG(0, "\nTesting query_server()\n");
        c_query_server();
        CDR_MSG(0, "\nTesting check_registration()\n");
        c_check_registration();  
        CDR_MSG(0, "\nTesting query_port()\n");
        c_query_port();
        CDR_MSG(0, "\nTesting define_pool(10, 100, 3, 1, 2, 200)\n");
        c_define_pool(10, 100, 3, 1, 2, 200);
        CDR_MSG(0, "\nTesting delete_pool(3, 1, 2, 200)\n");
        c_delete_pool(3, 1, 2, 200);
        CDR_MSG(0, "\nTesting query_mm_info()\n");
        c_query_mm_info(); 
        CDR_MSG(0, "\nTesting query_acs(0)\n");
        c_query_acs(0);
        CDR_MSG(0, "\nTesting query_acs(1, \"005\")\n");
        c_query_acs(1, "005");
        CDR_MSG(0, "\nTesting query_lsm(0)\n");
        c_query_lsm(0);
        CDR_MSG(0, "\nTesting query_drive_group(0)\n");
        c_query_drive_group(0);
        CDR_MSG(0, "\nTesting query_drive_group(2, \"DVTSS16\", \"DVTSS17\")\n");
        c_query_drive_group(2, "DVTSS16", "DVTSS17");
        CDR_MSG(0, "\nTesting query_cap(0)\n");
        c_query_cap(0);
        CDR_MSG(0, "\nTesting query_drive(0)\n");
        c_query_drive(0);
        CDR_MSG(0, "\nTesting query_subpool_name(0)\n");
        c_query_subpool_name(0);
        CDR_MSG(0, "\nTesting query_subpool_name(3, \"REAL1\", \"REAL2\", \"REAL3\")\n");
        c_query_subpool_name(3, "REAL1", "REAL2", "REAL3");
        CDR_MSG(0, "\nTesting query_pool(0)\n");
        c_query_pool(0);
        CDR_MSG(0, "\nTesting query_pool(3, 1, 200, 5)\n");
        c_query_pool(3, 1, 200, 5);
        CDR_MSG(0, "\nTesting query_scratch(0)\n");
        c_query_scratch(0);
        CDR_MSG(0, "\nTesting query_scratch(3, 1, 200, 5)\n");
        c_query_scratch(3, 1, 200, 5);
        CDR_MSG(0, "\nTesting query_clean()\n");
        c_query_clean();
        CDR_MSG(0, "\nTesting query_volume(0)\n");
        c_query_volume(0);
        CDR_MSG(0, "\nTesting query_volume(1, \"NOTFND\")\n");
        c_query_volume(1, "NOTFND");
        CDR_MSG(0, "\nTesting query_volume(1, \"DRL665\")\n");
        c_query_volume(1, "DRL665");
        CDR_MSG(0, "\nTesting query_volume(1, \"DX0010\")\n");
        c_query_volume(1, "DX0010");
        CDR_MSG(0, "\nTesting set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
        c_set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
        c_set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting set_clean(TRUE, 10, NO_LOCK_ID, 1, \"DRL001-DRL099\")\n");
        c_set_clean(TRUE, 10, NO_LOCK_ID, 1, "DRL001-DRL099");
        CDR_MSG(0, "\nTesting query_mount(1, \"DRL970\")\n");
        c_query_mount(1, "DRL970");
        CDR_MSG(0, "\nTesting query_mount(1, \"NOTFND\")\n");
        c_query_mount(1, "NOTFND");
        CDR_MSG(0, "\nTesting query_mount(1, \"DX0010\")\n");
        c_query_mount(1, "DX0010");
        CDR_MSG(0, "\nTesting query_mount_scratch(ANY_MEDIA_TYPE, 1, 10)\n");
        c_query_mount_scratch(ANY_MEDIA_TYPE, 1, 10);
        CDR_MSG(0, "\nTesting query_mount_scratch_pinfo(ANY_MEDIA_TYPE, \"MGMT1\", 1, 10)\n");
        c_query_mount_scratch_pinfo(ANY_MEDIA_TYPE, "MGMT1", 1, 10);
        CDR_MSG(0, "\nTesting audit server()\n");
        c_audit_server();
        CDR_MSG(0, "\nTesting audit acs()\n");
        c_audit_acs();
        CDR_MSG(0, "\nTesting audit lsm()\n");
        c_audit_lsm();
        CDR_MSG(0, "\nTesting audit panel()\n");
        c_audit_panel();
        CDR_MSG(0, "\nTesting audit subpanel()\n");
        c_audit_subpanel();
        CDR_MSG(0, "\nTesting cancel(201)\n");
        c_cancel(201);
        CDR_MSG(0, "\nTesting mount(0, \"DRL970\", \"000:001:009:006\", TRUE, TRUE)\n");
        c_mount(0, "DRL970", "000:001:009:006", TRUE, TRUE);
        CDR_MSG(0, "\nTesting dismount(0, \"DRL970\", \"000:001:009:006\", TRUE)\n");
        c_dismount(0, "DRL970", "000:001:009:006", TRUE);
        CDR_MSG(0, "\nTesting query_request(0)\n");
        c_query_request(0);
        CDR_MSG(0, "\nTesting display()\n");
        c_display();   
        CDR_MSG(0, "\nTesting eject(0, \"000:000:000\", 3, \"DRL001\", \"DRL002\", \"DRL003\")\n");
        c_eject(0, "000:000:000", 3, "DRL001", "DRL002", "DRL003");  
        CDR_MSG(0, "\nTesting xeject(0, \"000:000:000\", 2, \"DRL800-DRL810\", \"DRL900-DRL910\"\n");
        c_xeject(0, "000:000:000", 2, "DRL800-DRL810", "DRL900-DRL910");  
        CDR_MSG(0, "\nTesting enter(\"000:000:000\")\n");
        c_enter("000:000:000");  
        CDR_MSG(0, "\nTesting venter(\"000:000:000\"), 2, \"V00001\", \"V00002\"\n");
        c_venter("000:000:000", 2, "V00001", "V00002");  
        CDR_MSG(0, "\nTesting vary_acs(STATE_OFFLINE, TRUE, 1, \"000\")\n");
        c_vary_acs(STATE_OFFLINE, TRUE, 1, "000");
        CDR_MSG(0, "\nTesting vary_acs(STATE_ONLINE, FALSE, 1, \"000\")\n");
        c_vary_acs(STATE_ONLINE, FALSE, 1, "000");
        CDR_MSG(0, "\nTesting vary_cap(STATE_OFFLINE, 1, \"000:000:000\")\n");
        c_vary_cap(STATE_OFFLINE, 1, "000:000:000");
        CDR_MSG(0, "\nTesting vary_cap(STATE_ONLINE, 1, \"000:000:000\")\n");
        c_vary_cap(STATE_ONLINE, 1, "000:000:000");
        CDR_MSG(0, "\nTesting vary_drive(STATE_OFFLINE, 0, 1, \"001:000:010:000\")\n");
        c_vary_drive(STATE_OFFLINE, 0, 1, "001:000:010:000");
        CDR_MSG(0, "\nTesting vary_drive(STATE_ONLINE, 0, 1, \"001:000:010:000\")\n");
        c_vary_drive(STATE_ONLINE, 0, 1, "001:000:010:000");
        CDR_MSG(0, "\nTesting vary_lsm(STATE_OFFLINE, TRUE, 1, \"000:000\")\n");
        c_vary_lsm(STATE_OFFLINE, TRUE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_lsm(STATE_ONLINE, FALSE, 1, \"000:000\")\n");
        c_vary_lsm(STATE_ONLINE, FALSE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_port(STATE_OFFLINE, 1, \"000:000\")\n");
        c_vary_port(STATE_OFFLINE, 1, "000:000");
        CDR_MSG(0, "\nTesting vary_port(STATE_ONLINE, 1, \"000:000\")\n");
        c_vary_port(STATE_ONLINE, 1, "000:000");
        CDR_MSG(0, "\nTesting set_access(FALSE)\n");
        c_set_access(FALSE);  
    }

    /*****************************************************************/
    /* The following are examples of tailored ACSAPI routines.       */
    /*****************************************************************/
/*
    CDR_MSG(0, "\nTesting set_access(TRUE)\n");                                                 
    c_set_access(TRUE);  

    CDR_MSG(0, "\nTesting idle(FALSE)\n");
    c_idle(FALSE);

    CDR_MSG(0, "\nTesting start()\n");
    c_start();

    CDR_MSG(0, "\nTesting register()\n");
    c_register();

    CDR_MSG(0, "\nTesting query_server()\n");
    c_query_server();

    CDR_MSG(0, "\nTesting check_registration()\n");
    c_check_registration();  

    CDR_MSG(0, "\nTesting query_port()\n");
    c_query_port();

    CDR_MSG(0, "\nTesting define_pool(10, 100, 0)\n");
    c_define_pool(10, 100, 0);

    CDR_MSG(0, "\nTesting define_pool(10, 100, 3, 1, 2, 200)\n");
    c_define_pool(10, 100, 3, 1, 2, 200);

    CDR_MSG(0, "\nTesting delete_pool(0)\n");
    c_delete_pool(0);

    CDR_MSG(0, "\nTesting delete_pool(3, 1, 2, 200)\n");
    c_delete_pool(3, 1, 2, 200);

    CDR_MSG(0, "\nTesting query_mm_info()\n");
    c_query_mm_info(); 

    CDR_MSG(0, "\nTesting query_acs(0)\n");
    c_query_acs(0);

    CDR_MSG(0, "\nTesting query_acs(1, \"000\")\n");
    c_query_acs(1, "000");

    CDR_MSG(0, "\nTesting query_acs(1, \"005\")\n");
    c_query_acs(1, "005");

    CDR_MSG(0, "\nTesting query_lsm(0)\n");
    c_query_lsm(0);

    CDR_MSG(0, "\nTesting query_drive_group(0)\n");
    c_query_drive_group(0);

    CDR_MSG(0, "\nTesting query_drive_group(2, \"DVTSS16\", \"DVTSS17\")\n");
    c_query_drive_group(2, "DVTSS16", "DVTSS17");

    CDR_MSG(0, "\nTesting query_cap(0)\n");
    c_query_cap(0);

    CDR_MSG(0, "\nTesting query_drive(0)\n");
    c_query_drive(0);

    CDR_MSG(0, "\nTesting query_subpool_name(0)\n");
    c_query_subpool_name(0);

    CDR_MSG(0, "\nTesting query_subpool_name(3, \"REAL1\", \"REAL2\", \"REAL3\")\n");
    c_query_subpool_name(3, "REAL1", "REAL2", "REAL3");

    CDR_MSG(0, "\nTesting query_pool(0)\n");
    c_query_pool(0);

    CDR_MSG(0, "\nTesting query_pool(3, 1, 200, 5)\n");
    c_query_pool(3, 1, 200, 5);

    CDR_MSG(0, "\nTesting query_scratch(0)\n");
    c_query_scratch(0);

    CDR_MSG(0, "\nTesting query_scratch(3, 1, 200, 5)\n");
    c_query_scratch(3, 1, 200, 5);

    CDR_MSG(0, "\nTesting query_clean()\n");
    c_query_clean();

    CDR_MSG(0, "\nTesting query_volume(0)\n");
    c_query_volume(0);

    CDR_MSG(0, "\nTesting query_volume(1, \"NOTFND\")\n");
    c_query_volume(1, "NOTFND");

    CDR_MSG(0, "\nTesting query_volume(1, \"DRL665\")\n");
    c_query_volume(1, "DRL665");

    CDR_MSG(0, "\nTesting query_volume(1, \"DX0010\")\n");
    c_query_volume(1, "DX0010");

    CDR_MSG(0, "\nTesting set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
    c_set_scratch(TRUE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");

    CDR_MSG(0, "\nTesting set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, \"DRL001-DRL099\")\n");
    c_set_scratch(FALSE, NO_LOCK_ID, SAME_POOL, 1, "DRL001-DRL099");

    CDR_MSG(0, "\nTesting set_clean(TRUE, 10, NO_LOCK_ID, 1, \"DRL001-DRL099\")\n");
    c_set_clean(TRUE, 10, NO_LOCK_ID, 1, "DRL001-DRL099");

    CDR_MSG(0, "\nTesting query_mount(1, \"DRL970\")\n");
    c_query_mount(1, "DRL970");

    CDR_MSG(0, "\nTesting query_mount(1, \"NOTFND\")\n");
    c_query_mount(1, "NOTFND");

    CDR_MSG(0, "\nTesting query_mount(1, \"DX0010\")\n");
    c_query_mount(1, "DX0010");

    CDR_MSG(0, "\nTesting query_mount_scratch(ANY_MEDIA_TYPE, 1, 10)\n");
    c_query_mount_scratch(ANY_MEDIA_TYPE, 1, 10);

    CDR_MSG(0, "\nTesting query_mount_scratch_pinfo(ANY_MEDIA_TYPE, \"MGMT1\", 3, 10, 1, 120)\n");
    c_query_mount_scratch_pinfo(ANY_MEDIA_TYPE, "MGMT1", 3, 10, 1, 120);

    CDR_MSG(0, "\nTesting audit server()\n");
    c_audit_server();

    CDR_MSG(0, "\nTesting audit acs()\n");
    c_audit_acs();

    CDR_MSG(0, "\nTesting audit lsm()\n");
    c_audit_lsm();

    CDR_MSG(0, "\nTesting audit panel()\n");
    c_audit_panel();

    CDR_MSG(0, "\nTesting audit subpanel()\n");
    c_audit_subpanel();

    CDR_MSG(0, "\nTesting cancel(201)\n");
    c_cancel(201);

    CDR_MSG(0, "\nTesting query_drive(2, \"127:000:001:032\", \"000:001:009:006\")\n");
    c_query_drive(2, "127:000:001:032", "000:001:009:006");

    CDR_MSG(0, "\nTesting mount(0, \"DRL970\", \"000:001:009:006\", TRUE, TRUE)\n");
    c_mount(0, "DRL970", "000:001:009:006", TRUE, TRUE);

    CDR_MSG(0, "\nTesting dismount(0, \"DRL970\", \"000:001:009:006\", TRUE)\n");
    c_dismount(0, "DRL970", "000:001:009:006", TRUE);

    CDR_MSG(0, "\nTesting lock_drive(0, TRUE, 1, \"000:001:001:011\")\n");
    c_lock_drive(0, TRUE, 1, "000:001:001:011");

    CDR_MSG(0, "\nTesting query_lock_drive(%d, 1, \"000:001:001:011\")\n",
           lastLockId);
    c_query_lock_drive(lastLockId, 1, "000:001:001:011");

    CDR_MSG(0, "\nTesting lock_volume(%d, TRUE, 1, \"SA2520\")\n",
           lastLockId);
    c_lock_volume(lastLockId, TRUE, 1, "SA2520");

    CDR_MSG(0, "\nTesting query_lock_volume(%d, 1, \"SA2520\")\n",
           lastLockId);
    c_query_lock_volume(lastLockId, 1, "SA2520");

    CDR_MSG(0, "\nTesting mount(%d, \"SA2520\", \"000:001:001:011\", TRUE, TRUE)\n",
           lastLockId);
    c_mount(lastLockId, "SA2520", "000:001:001:011", TRUE, TRUE);

    CDR_MSG(0, "\nTesting dismount(%d, \"SA2520\", \"000:001:001:011\", TRUE)\n",
           lastLockId);
    c_dismount(lastLockId, "SA2520", "000:001:001:011", TRUE);

    CDR_MSG(0, "\nTesting unlock_drive(%d, 1, \"000:001:001:011\")\n",
           lastLockId);
    c_unlock_drive(lastLockId, 1, "000:001:001:011");

    CDR_MSG(0, "\nTesting query_lock_drive(%d, 1, \"000:001:001:011\")\n",
           lastLockId);
    c_query_lock_drive(lastLockId, 1, "000:001:001:011");

    CDR_MSG(0, "\nTesting clear_lock_drive(1, \"000:001:001:011\")\n");
    c_clear_lock_drive(1, "000:001:001:011");

    CDR_MSG(0, "\nTesting unlock_volume(%d, 1, \"SA2520\")\n",
           lastLockId);
    c_unlock_volume(lastLockId, 1, "SA2520");

    CDR_MSG(0, "\nTesting query_lock_volume(%d, 1, \"SA2520\")\n",
           lastLockId);
    c_query_lock_volume(lastLockId, 1, "SA2520");

    CDR_MSG(0, "\nTesting clear_lock_volume(1, \"SA2520\")\n");
    c_clear_lock_volume(1, "SA2520");
 
    CDR_MSG(0, "\nTesting query_volume(1, \"FB2988\")\n");
    c_query_volume(1, "FB2988");

    CDR_MSG(0, "\nTesting dismount(0, \"FB2988\", \"001:000:010:000\", FALSE)\n");
    c_dismount(0, "FB2988", "001:000:010:000", FALSE);

    CDR_MSG(0, "\nTesting mount_scratch(0, 2, ANY_MEDIA_TYPE, \"001:000:010:000\")\n");
    c_mount_scratch(0, 2, ANY_MEDIA_TYPE, "001:000:010:000");

    CDR_MSG(0, "\nTesting dismount(0, \" \", \"001:000:010:000\", TRUE)\n");
    c_dismount(0, " ", "001:000:010:000", TRUE);

    CDR_MSG(0, "\nTesting mount_pinfo(0, \" \", 2, \"MCLASS01\", "
           "ANY_MEDIA_TYPE, TRUE, FALSE, FALSE, "
           "\"SMC0JOBX\", \" \", \" \", \"001:000:010:000\")\n");
    c_mount_pinfo(0, " ", 2, "MCLASS01", 
                  ANY_MEDIA_TYPE, TRUE, FALSE, FALSE,
                  "SMC0JOBX", " ", " ", "001:000:010:000");

    CDR_MSG(0, "\nTesting dismount(0, \" \", \"001:000:010:000\", TRUE)\n");
    c_dismount(0, " ", "001:000:010:000", TRUE);

    CDR_MSG(0, "\nTesting lock_drive(0, TRUE, 1, \"000:000:009:001\")\n");
    c_lock_drive(0, TRUE, 1, "000:000:009:001");

    CDR_MSG(0, "\nTesting lock_drive(9999, TRUE, 0)\n");
    c_lock_drive(9999, TRUE, 0);

    CDR_MSG(0, "\nTesting lock_drive(9999, TRUE, 1, \"000:000:009:001\")\n");
    c_lock_drive(9999, TRUE, 1, "000:000:009:001");

    CDR_MSG(0, "\nTesting lock_drive(0, TRUE, 2, \"000:000:009:002\", \"000:000:009:004\")\n");
    c_lock_drive(0, TRUE, 2, "000:000:009:002", "000:000:009:004");

    CDR_MSG(0, "\nTesting query_lock_drive(0, 0)\n");
    c_query_lock_drive(0, 0);

    CDR_MSG(0, "\nTesting unlock_drive(1, 0)\n");
    c_unlock_drive(1, 0);

    CDR_MSG(0, "\nTesting query_lock_volume(0, 0)\n");
    c_query_lock_volume(0, 0);

    CDR_MSG(0, "\nTesting query_request(3, 500, 501, 502)\n");
    c_query_request(3, 500, 501, 502);

    CDR_MSG(0, "\nTesting query_request(0)\n");
    c_query_request(0);

    CDR_MSG(0, "\nTesting display()\n");
    c_display();   

    CDR_MSG(0, "\nTesting eject(0, \"000:000:000\", 3, \"DRL001\", \"DRL002\", \"DRL003\")\n");
    CDR_MSG(0, "EJECT requires manual intervention until CAP RELEASE\n");
    CDR_MSG(0, "Reply 'C' to cancel EJECT or anything else to continue: ");
    scanf("%s", reply);
    if (toupper(reply[0]) != 'C')
    {
        c_eject(0, "000:000:000", 3, "DRL001", "DRL002", "DRL003");  
    }

    CDR_MSG(0, "\nTesting xeject(0, \"000:000:000\", 2, \"DRL800-DRL810\", \"DRL900-DRL910\"\n");
    CDR_MSG(0, "XEJECT requires manual intervention until CAP RELEASE\n");
    CDR_MSG(0, "Reply 'C' to cancel XEJECT or anything else to continue: ");
    scanf("%s", reply);
    if (toupper(reply[0]) != 'C')
    {
        c_xeject(0, "000:000:000", 2, "DRL800-DRL810", "DRL900-DRL910");  
    }

    CDR_MSG(0, "\nTesting enter(\"000:000:000\")\n");
    CDR_MSG(0, "ENTER requires manual intervention until CAP RELEASE\n");
    CDR_MSG(0, "Reply 'C' to cancel ENTER or anything else to continue: ");
    scanf("%s", reply);
    if (toupper(reply[0]) != 'C')
    {
        c_enter("000:000:000");  
    }

    CDR_MSG(0, "\nTesting venter(\"000:000:000\"), 2, \"V00001\", \"V00002\"\n");
    CDR_MSG(0, "VENTER requires manual intervention until CAP RELEASE\n");
    CDR_MSG(0, "Reply 'C' to cancel VENTER or anything else to continue: ");
    scanf("%s", reply);
    if (toupper(reply[0]) != 'C')
    {
        c_venter("000:000:000", 2, "V00001", "V00002");  
    }

    CDR_MSG(0, "\nTesting set_cap(5, CAP_MODE_SAME, 1, \"000:000:000\")\n");
    c_set_cap(5, CAP_MODE_SAME, 1, "000:000:000");

    CDR_MSG(0, "\nTesting vary_acs(STATE_OFFLINE, TRUE, 1, \"000\")\n");
    c_vary_acs(STATE_OFFLINE, TRUE, 1, "000");

    CDR_MSG(0, "\nTesting vary_acs(STATE_ONLINE, FALSE, 1, \"000\")\n");
    c_vary_acs(STATE_ONLINE, FALSE, 1, "000");

    CDR_MSG(0, "\nTesting vary_cap(STATE_OFFLINE, 1, \"000:000:000\")\n");
    c_vary_cap(STATE_OFFLINE, 1, "000:000:000");

    CDR_MSG(0, "\nTesting vary_cap(STATE_ONLINE, 1, \"000:000:000\")\n");
    c_vary_cap(STATE_ONLINE, 1, "000:000:000");

    CDR_MSG(0, "\nTesting vary_drive(STATE_OFFLINE, 9999, 1, \"001:000:010:000\")\n");
    c_vary_drive(STATE_OFFLINE, 9999, 1, "001:000:010:000");

    CDR_MSG(0, "\nTesting vary_drive(STATE_ONLINE, 9999, 1, \"001:000:010:000\")\n");
    c_vary_drive(STATE_ONLINE, 9999, 1, "001:000:010:000");

    CDR_MSG(0, "\nTesting vary_lsm(STATE_OFFLINE, TRUE, 1, \"000:000\")\n");
    c_vary_lsm(STATE_OFFLINE, TRUE, 1, "000:000");

    CDR_MSG(0, "\nTesting vary_lsm(STATE_ONLINE, FALSE, 1, \"000:000\")\n");
    c_vary_lsm(STATE_ONLINE, FALSE, 1, "000:000");

    CDR_MSG(0, "\nTesting vary_port(STATE_OFFLINE, 1, \"000:000\")\n");
    c_vary_port(STATE_OFFLINE, 1, "000:000");

    CDR_MSG(0, "\nTesting vary_port(STATE_ONLINE, 1, \"000:000\")\n");
    c_vary_port(STATE_ONLINE, 1, "000:000");

    CDR_MSG(0, "\nTesting set_access(FALSE)\n");
    c_set_access(FALSE);  
*/
    return;
}


/*********************************************************************/
/* The client routines start here.                                   */
/*********************************************************************/

#undef SELF
#define SELF "c_audit_acs"
void c_audit_acs(void)
{
    ACS_AUDIT_ACS_RESPONSE *afp;
    CAPID               cap_id;
    ACS                 acs[MAX_ID];
    unsigned short      i;

    s = (SEQ_NO)101;
    cap_id.lsm_id.acs = 0;
    cap_id.lsm_id.lsm = 0;
    cap_id.cap = 0;
    i = 1;
    acs[0] = 0;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_audit_acs(s, acs, cap_id, i);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_audit_acs() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            st_show_audit_int_res(SELF);
        }
        else if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            afp = (ACS_AUDIT_ACS_RESPONSE *) rbuf;

            /* Leave non-check_mode code for possible later use */
            if (!check_mode)
            {
                CDR_MSG(4, "\t%s: audit status: %s\n", 
                        SELF, acs_status(afp->audit_acs_status));

                for (i = 0; i < afp->count; i++)
                {
                    CDR_MSG(4, "\t%s: acs = %d\n", SELF, afp->acs[i]);
                    CDR_MSG(4, "\t%s: acs status: %s\n", SELF, 
                            acs_status(afp->acs_status[i]));
                }
            }
            else
            {
                if (afp->audit_acs_status != STATUS_SUCCESS)
                {
                    CDR_MSG(0, "\t%s: FINAL RESPONSE audit_acs_status failure\n", SELF);
                }

                for (i = 0; i < afp->count; i++)
                {
                    if ((afp->acs[i] != acs[i]) || (afp->acs_status[i] != 
                                                    STATUS_VALID))
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE acs failure\n", SELF);
                    }
                }
            }

            break;
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_audit_lsm"
void c_audit_lsm()
{
    ACS_AUDIT_LSM_RESPONSE *afp;
    unsigned short      i;
    CAPID               cap_id;
    LSMID               lsm_id[MAX_ID];
    int                 count;

    s = (SEQ_NO)102;
    cap_id.lsm_id.acs = 0;
    cap_id.lsm_id.lsm = 0;
    cap_id.cap = 0;
    lsm_id[0].acs = 3;
    lsm_id[0].lsm = 4;
    count = 1;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_audit_lsm(s, lsm_id, cap_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_audit_lsm() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            st_show_audit_int_res(SELF);
        }
        else if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            afp = (ACS_AUDIT_LSM_RESPONSE *) rbuf;

            /* Leave non-check_mode code for possible later use */
            if (!check_mode)
            {
                CDR_MSG(4, "\tFinal Status: %s\n", acs_status(afp->audit_lsm_status));

                if (afp->audit_lsm_status == STATUS_SUCCESS)
                {
                    for (i = 0; i < afp->count; i++)
                    {
                        st_show_lsm_id(i, 
                                       &(afp->lsm_status[i]), 
                                       &(afp->lsm_id[i]));
                    }
                }
            }
            else
            {
                if (afp->audit_lsm_status != STATUS_SUCCESS)
                {
                    CDR_MSG(0, "\t%s: FINAL RESPONSE audit_lsm_status failure\n", SELF);
                    CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(afp->audit_lsm_status));
                }
                else
                {
                    for (i = 0; i < afp->count; i++)
                    {
                        st_chk_lsm_id(i, 
                                      &(afp->lsm_status[i]), 
                                      STATUS_VALID, 
                                      &(afp->lsm_id[i]), 
                                      &(lsm_id[i]));
                    }
                }
            }
            break;
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_audit_panel"
void c_audit_panel()
{
    ACS_AUDIT_PNL_RESPONSE *afp;
    unsigned short      i;
    CAPID               cap_id;
    PANELID             panel_id[MAX_ID];
    int                 count;

    s = (SEQ_NO)103;
    cap_id.lsm_id.acs = 0;
    cap_id.lsm_id.lsm = 0;
    cap_id.cap = 0;
    panel_id[0].lsm_id.acs = 3;
    panel_id[0].lsm_id.lsm = 4;
    panel_id[0].panel = 5;
    count = 1;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_audit_panel(s, panel_id, cap_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_audit_panel() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            st_show_audit_int_res(SELF);
        }
        else if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            afp = (ACS_AUDIT_PNL_RESPONSE *) rbuf;

            /* Leave non-check_mode code for possible later use */
            if (!check_mode)
            {
                CDR_MSG(4, "\t%s: Status: %s\n", SELF, 
                        acs_status(afp->audit_pnl_status));

                if (afp->audit_pnl_status == STATUS_SUCCESS)
                {
                    for (i = 0; i < afp->count; i++)
                    {
                        st_show_panel_info(i, 
                                           &(afp->panel_status[i]), 
                                           &(afp->panel_id[i]));
                    }
                }
            }
            else
            {
                if (afp->audit_pnl_status != STATUS_SUCCESS)
                {
                    CDR_MSG(0, "\t%s: FINAL RESPONSE audit_pnl_status failure\n", SELF);
                    CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(afp->audit_pnl_status));
                }
                else
                {
                    for (i = 0; i < afp->count; i++)
                    {
                        st_chk_panel_info(i, 
                                          &(afp->panel_status[i]), 
                                          STATUS_VALID, 
                                          &(afp->panel_id[i]), 
                                          &(panel_id[i]));
                    }
                }
            }
            break;
        }
    }while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_audit_server"
void c_audit_server()
{
    ACS_AUDIT_SRV_RESPONSE *afp;
    CAPID       cap_id;

    s = (SEQ_NO)303;
    cap_id.lsm_id.acs = 0;
    cap_id.lsm_id.lsm = 0;
    cap_id.cap = 0;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_audit_server(s, cap_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_audit_server() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            st_show_audit_int_res(SELF);
        }
        else if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }
            afp = (ACS_AUDIT_SRV_RESPONSE *) rbuf;

            /* Leave non-check_mode code for possible later use */
            if (!check_mode)
            {
                CDR_MSG(4, "\t%s: status: %s\n", 
                        SELF, acs_status(afp->audit_srv_status));
            }
            else
            {
                if (afp->audit_srv_status != STATUS_SUCCESS)
                {
                    CDR_MSG(0, "\t%s: FINAL RESPONSE audit_acs_status failure\n", SELF);
                    CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(afp->audit_srv_status));
                }
            }

            break;
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_audit_subpanel"
void c_audit_subpanel()
{
    ACS_AUDIT_SUB_RESPONSE *afp;
    unsigned short      i;
    CAPID               cap_id;
    SUBPANELID          subpanel_id[MAX_ID];
    unsigned short      count;

    s = (SEQ_NO)104;
    cap_id.lsm_id.acs = 0;
    cap_id.lsm_id.lsm = 0;
    cap_id.cap = 0;
    subpanel_id[0].panel_id.lsm_id.acs = 3;
    subpanel_id[0].panel_id.lsm_id.lsm = 4;
    subpanel_id[0].panel_id.panel = 5;
    subpanel_id[0].begin_row = 6;
    subpanel_id[0].begin_col = 0;
    subpanel_id[0].end_row = 0;
    subpanel_id[0].end_col = 1;
    count = 1;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_audit_subpanel(s, subpanel_id, cap_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_audit_subpanel() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            st_show_audit_int_res(SELF);
        }
        else if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }
            afp = (ACS_AUDIT_SUB_RESPONSE *) rbuf;

            /* Leave non-check_mode code for possible later use */
            if (!check_mode)
            {
                CDR_MSG(4, "\tFinal Status: %s\n", acs_status(afp->audit_sub_status));

                if (afp->audit_sub_status == STATUS_SUCCESS)
                {
                    for (i = 0; i < (afp->count); i++)
                    {
                        CDR_MSG(4, "\tsubpanel #%d: status is %s\n", 
                                i, acs_status(afp->subpanel_status[i]));
                        CDR_MSG(4, "\t          acs is = %d\n", 
                                afp->subpanel_id[i].panel_id.lsm_id.acs);
                        CDR_MSG(4, "\t          lsm is = %d\n", 
                                afp->subpanel_id[i].panel_id.lsm_id.lsm);
                        CDR_MSG(4, "\t          panel is = %d\n", 
                                afp->subpanel_id[i].panel_id.lsm_id.lsm);
                        CDR_MSG(4, "\t          begin row is = %d\n", 
                                afp->subpanel_id[i].begin_row);
                        CDR_MSG(4, "\t          begin column is = %d\n", 
                                afp->subpanel_id[i].begin_col);
                        CDR_MSG(4, "\t          end row is = %d\n", 
                                afp->subpanel_id[i].end_row);
                        CDR_MSG(4, "\t          end column is = %d\n", 
                                afp->subpanel_id[i].end_col);
                    }
                }
            }
            else
            {
                if (afp->audit_sub_status != STATUS_SUCCESS)
                {
                    CDR_MSG(0, "\t%s: FINAL RESPONSE audit_sub_status failure\n", SELF);
                    CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(afp->audit_sub_status));
                }
                else
                {
                    for (i = 0; i < afp->count; i++)
                    {
                        if (afp->subpanel_id[i].panel_id.lsm_id.acs != 
                            subpanel_id[i].panel_id.lsm_id.acs)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE acs failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].panel_id.lsm_id.lsm != 
                            subpanel_id[i].panel_id.lsm_id.lsm)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE lsm failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].panel_id.panel != 
                            subpanel_id[i].panel_id.panel)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE panel failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].begin_row
                            != subpanel_id[i].begin_row)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE begin_row failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].begin_col
                            != subpanel_id[i].begin_col)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE begin_col failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].end_row
                            != subpanel_id[i].end_row)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE end_row failure\n", SELF);
                        }

                        if (afp->subpanel_id[i].end_col
                            != subpanel_id[0].end_col)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE end_col failure\n", SELF);
                        }
                    }
                }
            }
            break;
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_cancel"
/*********************************************************************/
/* Input integer SEQ_NO (i.e. packet id) of request to cancel.       */
/*********************************************************************/
void c_cancel(int cancelId)
{
    ACS_CANCEL_RESPONSE *cp;

    s = (SEQ_NO) cancelId;
    status = acs_cancel(s, req_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_cancel() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            cp = (ACS_CANCEL_RESPONSE *) rbuf;

            if (cp->cancel_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE cancel_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(cp->cancel_status));
            }

            if (cp->req_id != req_id)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE req_id failure\n", SELF);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_check_registration"
void c_check_registration()
{
    int                 i;
    unsigned short      count;
    REGISTRATION_ID     registration_id;
    EVENT_REGISTER_STATUS *ev_reg_stat;
    ACS_CHECK_REGISTRATION_RESPONSE *from_server;

    s = (SEQ_NO)405;
    strcpy(registration_id.registration, "reg_test");

    status = acs_check_registration(s, registration_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_check_registration() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            from_server = (ACS_CHECK_REGISTRATION_RESPONSE *) rbuf;

            if (from_server->check_registration_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE check_registration_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(from_server->check_registration_status));
            }

            st_show_event_register_status(&from_server->event_register_status);
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_clear_lock_drive"
/*********************************************************************/
/* Input count of drives to clear locks, plus driveId string(s).     */
/* A count of 0 should return an error.                              */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_clear_lock_drive(int count, ...)
{
    DRIVEID             drive_id[MAX_ID];
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    ACS_CLEAR_LOCK_DRV_RESPONSE *cp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);

    s = (SEQ_NO)251;
    status = acs_clear_lock_drive(s, drive_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_clear_lock_drive() failed %s\n", SELF, acs_status(status));
        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            cp = (ACS_CLEAR_LOCK_DRV_RESPONSE *) rbuf;

            if (cp->clear_lock_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE clear_lock_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: FINAL RESPONSE status = %s\n", 
                        SELF, acs_status(cp->clear_lock_drv_status));

                return;
            }
            else
            {
                st_show_lo_drv_status(cp->count, 
                                      &(cp->drv_status[0]), 
                                      &(drive_id[0]),
                                      SELF);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_clear_lock_volume"
/*********************************************************************/
/* Input count of volser(s) to clear locks, plus volser string(s).   */
/* NOTE: An count of 0 should return an error.                       */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_clear_lock_volume(int count, ...)
{
    VOLID               vol_id[MAX_ID];
    char               *pVolIdString;
    int                 i;
    va_list             volIds;
    ACS_CLEAR_LOCK_VOL_RESPONSE *cp;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO) 252;

    status = acs_clear_lock_volume(s, vol_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_clear_lock_volume() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            cp = (ACS_CLEAR_LOCK_VOL_RESPONSE *) rbuf;

            if (cp->clear_lock_vol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE clear_lock_vol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: FINAL RESPONSE status = %s\n", 
                        SELF, acs_status(cp->clear_lock_vol_status));

                return;
            }
            else
            {
                st_show_lo_vol_status(cp->count, 
                                      &(cp->vol_status[0]), 
                                      &(vol_id[0]),
                                      SELF);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_define_pool"
/*********************************************************************/
/* Input low water mark, plus high water mark, plus count of number  */
/* of pools to define, plus integers representing each scratch       */
/* subpool index to define.                                          */
/*                                                                   */
/* If invoked under the XAPI, then the define becomes a query.       */
/* Count = 0 should return an error.                                 */
/*********************************************************************/
void c_define_pool(unsigned long lowWater, 
                   unsigned long highWater, 
                   int           count, ...)
{
    int                 i;
    unsigned long       attributes          = OVERFLOW;
    POOL                pool[MAX_ID];
    va_list             poolIds;
    ACS_DEFINE_POOL_RESPONSE *dp;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)261;
    status = acs_define_pool(s, lowWater, highWater, attributes, pool, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_define_pool() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            dp = (ACS_DEFINE_POOL_RESPONSE *) rbuf;

            if (dp->define_pool_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE define_pool_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(dp->define_pool_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < dp->count; i++)
                    {
                        st_show_pool_misc(i, 
                                          dp->lwm, 
                                          dp->hwm, 
                                          dp->attributes);

                        st_show_pool_info(i, 
                                          &(dp->pool_status[i]), 
                                          &(dp->pool[i]));
                    }
                }
                else
                {
                    st_chk_pool_misc(dp->lwm, 
                                     lowWater, 
                                     dp->hwm, 
                                     highWater, 
                                     dp->attributes, attributes);

                    for (i = 0; i < dp->count; i++)
                    {
                        st_chk_pool_info(i, 
                                         &(dp->pool[i]), 
                                         &(dp->pool_status[i]), 
                                         &(pool[i]), 
                                         STATUS_POOL_LOW_WATER);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_delete_pool"
/*********************************************************************/
/* Input number of integer pools, plus additional integers           */
/* representing each scratch subpool index to delete.                */
/*********************************************************************/
void c_delete_pool(int count, ...)
{
    int                 i;
    POOL                pool[MAX_ID];
    va_list             poolIds;
    ACS_DELETE_POOL_RESPONSE *dp;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)261;

    status = acs_delete_pool(s, pool, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_delete_pool() failed %s\n", SELF, 
                acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            dp = (ACS_DELETE_POOL_RESPONSE *) rbuf;

            if (dp->delete_pool_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE delete_pool_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(dp->delete_pool_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < dp->count; i++)
                    {
                        st_show_pool_info(i, 
                                          &(dp->pool_status[i]), 
                                          &(dp->pool[i]));
                    }
                }
                else
                {
                    for (i = 0; i < dp->count; i++)
                    {
                        st_chk_pool_info(i, 
                                         &(dp->pool[i]), 
                                         &(dp->pool_status[i]), 
                                         &(pool[i]), 
                                         STATUS_SUCCESS);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_dismount"
/*********************************************************************/
/* Input integer LOCKID or 0 or NO_LOCK_ID to ignore, plus volser    */
/* to mount, plus driveId, plus BOOLEAN value indicating whether     */
/* volume should be unloaded before dismount is attempted.           */
/* If invoked under the XAPI, and the drive is a VTD, then the       */
/* BOOLEAN force flag is ignored.                                    */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_dismount(int     lockId, 
                char   *volidString, 
                char    driveId[15], 
                BOOLEAN force)
{
    LOCKID              lock_id;
    VOLID               vol_id;
    DRIVEID             drive_id;
    int                 wkInt;
    char                driveIdSubstring[4];
    ACS_DISMOUNT_RESPONSE *dp;
    ACS_MOUNT_RESPONSE *mp;

    s = (SEQ_NO)301;
    lock_id = (LOCKID) lockId;
    strcpy(vol_id.external_label, volidString);

    /*****************************************************************/
    /* Input driveId is in format ACS:LSM:PAN:DRV (with each element */
    /* being 3 digits, separated by ":").                            */
    /* You can specify a virtual drive if you know the dummy         */
    /* ACS:LSM:PAN:DRV specification.                                */
    /*****************************************************************/
    memset(driveIdSubstring, 0, sizeof(driveIdSubstring));
    memcpy(driveIdSubstring, &(driveId[0]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.acs = (ACS) wkInt;

    memcpy(driveIdSubstring, &(driveId[4]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(driveIdSubstring, &(driveId[8]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.panel = (PANEL) wkInt;

    memcpy(driveIdSubstring, &(driveId[12]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.drive = (DRIVE) wkInt;

    status = acs_dismount(s, lock_id, vol_id, drive_id, force);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_dismount() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE | FORCE, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            dp = (ACS_DISMOUNT_RESPONSE *) rbuf;

            if (dp->dismount_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE dismount_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(dp->dismount_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_vol_info(0, 
                                     &(dp->dismount_status), 
                                     &(dp->vol_id));

                    st_show_drv_info(0, 
                                     NULL, 
                                     &(dp->drive_id));
                }
                else
                {
                    st_chk_vol_info(0, 
                                    &(dp->dismount_status), 
                                    STATUS_SUCCESS, 
                                    &(dp->vol_id), 
                                    &(vol_id));

                    st_chk_drv_info(0, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(dp->drive_id), 
                                    &(drive_id));
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_display"
void c_display()
{
    ACS_DISPLAY_RESPONSE *from_server;
    int                 i;
    TYPE                display_type;
    DISPLAY_XML_DATA    display_xml_data;
    char example1[400] = "<request type=\"DISPLAY\"><gettypenames/></request>";
    char example2[400] = "<request type=\"DISPLAY\"><getview><view name=\"viewname\"/></getview></request>";
    char dBegin[]      = "<request type=\"DISPLAY\"><display><token>display</token>";
    char dEnd[]        = "</display></request>";
    char dTokBegin[]   = "<token>";
    char dTokEnd[]     = "</token>";
    char token[100];
    char dExample[MAX_XML_DATA_SIZE];

    /* This example demonstrates how to build a properly formatted display command. */
    /* Other commands can be composed by simply replacing the ACS component with    */
    /* a component name of your choice (e.g. acs, lsm, cap, panel, etc.), and then  */
    /* replace the ID with an identifier that is valid for that component.  Full    */
    /* detailed listings of components may be found in the display command          */
    /* documentation in the CDK or ACSLS manuals.                                   */

    memset(dExample, 0, MAX_XML_DATA_SIZE);
    strcat(dExample, dBegin);
    sprintf(token, "%s%s%s", dTokBegin, "lsm", dTokEnd);
    strcat(dExample, token);
    sprintf(token, "%s%s%s", dTokBegin, "0,0", dTokEnd);
    strcat(dExample, token);
    sprintf(token, "%s%s%s", dTokBegin, "0,1", dTokEnd);
    strcat(dExample, token);
    strcat(dExample, dEnd);

    s = (SEQ_NO)449;
    display_xml_data.length=strlen(dExample);
    display_type=TYPE_DISPLAY;
    strcpy(display_xml_data.xml_data, dExample);

    status = acs_display(s, display_type, display_xml_data);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_display() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else if (type == RT_FINAL)
            {
                st_show_final_resp_hdr();
            }

            from_server = (ACS_DISPLAY_RESPONSE *) rbuf;

            if (from_server->display_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE display_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(from_server->display_status));
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            if (!check_mode)
            {
                st_show_display_info(from_server);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_eject"
/*********************************************************************/
/* Input integer LOCKID or 0 or NO_LOCK_ID to ignore, plus CAPID,    */
/* plus number of volumes to eject, plus volsers of volumes to       */
/* eject.  LOCKID is assumed to apply to volser(s).                  */
/*                                                                   */
/* Format of input capId is:                                         */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_eject(int  lockId, 
             char capId[11], 
             int  count, ...)
{
    VOLID               vol_id[MAX_ID];
    LOCKID              lock_id = (LOCKID) lockId;
    CAPID               cap_id;
    char               *pVolIdString;
    int                 i;
    int                 wkInt;
    char                capIdSubstring[4];
    va_list             volIds;
    ACS_EJECT_RESPONSE *ep;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    memset(capIdSubstring, 0 , sizeof(capIdSubstring));
    memcpy(capIdSubstring, &(capId[0]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.acs = (ACS) wkInt;

    memcpy(capIdSubstring, &(capId[4]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(capIdSubstring, &(capId[8]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.cap = (CAP) wkInt;

    s = (SEQ_NO)401;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_eject(s, lock_id, cap_id, count, vol_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_eject() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else if (type == RT_FINAL)
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            ep = (ACS_EJECT_RESPONSE *) rbuf;

            if (ep->eject_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE eject_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(ep->eject_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_cap_requested(&(ep->cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(ep->vol_status[i]), 
                                         &(ep->vol_id[i]));

                        if (ep->vol_status[i] == STATUS_SUCCESS)
                        {
                            st_show_cap_info(-1, 
                                             NULL, 
                                             &(ep->cap_used[i]));
                        }
                    }
                }
                else
                {
                    st_chk_cap_info(ep->count, 
                                    count, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(ep->cap_id), 
                                    &(cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        st_chk_vol_info(i, 
                                        &(ep->vol_status[i]), 
                                        STATUS_SUCCESS, 
                                        &(ep->vol_id[i]), 
                                        &(volumes[i]));

                        st_chk_cap_info(ep->count, 
                                        count, 
                                        NULL, 
                                        STATUS_SUCCESS, 
                                        &(ep->cap_used[i]), 
                                        &(cap_id));
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_enter"
/*********************************************************************/
/* Input CAPID for ENTER.                                            */
/*                                                                   */
/* Format of input capId is:                                         */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*********************************************************************/
void c_enter(char capId[11])
{
    int                 i;
    int                 wkInt;
    char                capIdSubstring[4];
    CAPID               cap_id;
    ACS_ENTER_RESPONSE *ep;
    VOLID               vol_id;
    char                tbuf[EXTERNAL_LABEL_SIZE + 1];

    memset(capIdSubstring, 0 , sizeof(capIdSubstring));
    memcpy(capIdSubstring, &(capId[0]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.acs = (ACS) wkInt;

    memcpy(capIdSubstring, &(capId[4]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(capIdSubstring, &(capId[8]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.cap = (CAP) wkInt;

    s = (SEQ_NO)402;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_enter(s, cap_id, FALSE);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_enter() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else if (type == RT_FINAL)
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            ep = (ACS_ENTER_RESPONSE *) rbuf;

            if (ep->enter_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE enter_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(ep->enter_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_cap_info(0, 
                                     NULL, 
                                     &(ep->cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(ep->vol_status[i]), 
                                         &(ep->vol_id[i]));
                    }
                }
                else
                {
                    st_chk_cap_info(ep->count, 
                                    MAX_ID, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(ep->cap_id), 
                                    &(cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        sprintf(tbuf, "SPE0%02d", i);
                        strcpy(&(vol_id.external_label[0]), &(tbuf[0]));

                        st_chk_vol_info(i, 
                                        &(ep->vol_status[i]), 
                                        STATUS_SUCCESS, 
                                        &(ep->vol_id[i]), 
                                        &(vol_id));
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_idle"
/*********************************************************************/
/* Input BOOLEAN value to indicate whether executing requests are    */
/* to be cancelled as part of IDLE request, or whether executing     */
/* requests can complete normally.                                   */
/*********************************************************************/
void c_idle(BOOLEAN force)
{
    ACS_IDLE_RESPONSE *ip;

    s = (SEQ_NO)011;

    status = acs_idle(s, force);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_idle() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, (force ? EXTENDED | ACKNOWLEDGE | FORCE : EXTENDED | ACKNOWLEDGE), 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            ip = (ACS_IDLE_RESPONSE *) rbuf;

            if (ip->idle_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE idle_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(ip->idle_status));
            }
        }                               
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_lock_drive"
/*********************************************************************/
/* Input integer LOCKID, plus BOOLEAN value to indicate whether we   */
/* wait for drives (if already locked) or return immediately,        */
/* plus integer number of drives to lock, plus driveId(s) of         */
/* drives to lock.                                                   */
/* NOTE: An input LOCKID of 0 or NO_LOCK_ID should be accepted with  */
/* returned lock_id indicating lock_id used.                         */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_lock_drive(int     lockId, 
                  BOOLEAN wait, 
                  int     count, ...)
{
    LOCKID              lock_id             = (LOCKID) lockId;
    USERID              user_id;
    DRIVEID             drive_id[MAX_ID];
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    ACS_LOCK_DRV_RESPONSE *lp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);

    memset((char*) &(user_id), 0, sizeof(USERID));

    if ((getenv("ACSAPI_USER_ID")) == NULL)
    {
        strcpy(user_id.user_label, "t_cdriver");
    }
    else
    {
        strcpy(user_id.user_label, getenv("ACSAPI_USER_ID"));
    }

    s = (SEQ_NO)351;
    status = acs_lock_drive(s, lock_id, user_id, drive_id, wait, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_lock_drive() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, WAIT, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            lp = (ACS_LOCK_DRV_RESPONSE *) rbuf;

            if (lp->lock_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE lock_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: FINAL RESPONSE status = %s\n", 
                        SELF, acs_status(lp->lock_drv_status));

                return;
            }
            else
            {
                lastLockId = (int) lp->lock_id;
                CDR_MSG(4, "\t%s: Returned lock_id=%d\n", SELF, lastLockId);

                st_show_lo_drv_status(lp->count, 
                                      &(lp->drv_status[0]), 
                                      &(drive_id[0]),
                                      SELF);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_lock_volume"
/*********************************************************************/
/* Input integer LOCKID, plus BOOLEAN wait flag, plus integer        */
/* count of volser(s) to query, plus volser string(s).               */
/* NOTE: An input LOCKID of 0 or NO_LOCK_ID should be accepted with  */
/* returned lock_id indicating lock_id used.                         */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_lock_volume(int     lockId, 
                   BOOLEAN wait,
                   int     count, ...)
{
    VOLID               vol_id[MAX_ID];
    LOCKID              lock_id = (LOCKID) lockId;
    USERID              user_id;
    char               *pVolIdString;
    int                 i;
    va_list             volIds;
    ACS_LOCK_VOL_RESPONSE *lp;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO) 352;

    memset((char*) &(user_id), 0, sizeof(USERID));

    if ((getenv("ACSAPI_USER_ID")) == NULL)
    {
        strcpy(user_id.user_label, "t_cdriver");
    }
    else
    {
        strcpy(user_id.user_label, getenv("ACSAPI_USER_ID"));
    }

    status = acs_lock_volume(s, lock_id, user_id, vol_id, wait, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_lock_volume() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, WAIT, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            lp = (ACS_LOCK_VOL_RESPONSE *) rbuf;

            if (lp->lock_vol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE lock_vol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(lp->lock_vol_status));
            }
            else
            {
                lastLockId = (int) lp->lock_id;
                CDR_MSG(4, "\t%s: Returned lock_id=%d\n", SELF, lastLockId);

                st_show_lo_vol_status(lp->count, 
                                      &(lp->vol_status[0]), 
                                      &(vol_id[0]),
                                      SELF);
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_mount"
/*********************************************************************/
/* Input integer LOCKID or 0 or NO_LOCK_ID to ignore, plus volser to */
/* mount, plus driveId to mount volser, plus BOOLEAN value to        */
/* indicate whether volume is mounted read-only, plus BOOLEAN value  */
/* to indicate whether volume is unlabelled (i.e. bypass volume      */
/* label checking for real tape cartridges with unreadable label).   */
/* NOTE: If invoked under the XAPI, then the BOOLEAN bypass flag     */
/* is ignored.                                                       */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_mount(int     lockId, 
             char   *volIdString, 
             char    driveId[15], 
             BOOLEAN readonly, 
             BOOLEAN bypass)
{
    LOCKID              lock_id = (LOCKID) lockId;
    VOLID               vol_id;
    DRIVEID             drive_id;
    int                 wkInt;
    char                driveIdSubstring[4];
    ACS_MOUNT_RESPONSE *mp;

    strcpy(vol_id.external_label, volIdString);

    /*****************************************************************/
    /* Input driveId is in format ACS:LSM:PAN:DRV (with each element */
    /* being 3 digits, separated by ":").                            */
    /* You can specify a virtual drive if you know the dummy         */
    /* ACS:LSM:PAN:DRV specification.                                */
    /*****************************************************************/
    memset(driveIdSubstring, 0, sizeof(driveIdSubstring));
    memcpy(driveIdSubstring, &(driveId[0]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.acs = (ACS) wkInt;

    memcpy(driveIdSubstring, &(driveId[4]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(driveIdSubstring, &(driveId[8]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.panel = (PANEL) wkInt;

    memcpy(driveIdSubstring, &(driveId[12]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.drive = (DRIVE) wkInt;

    s = (SEQ_NO)302;
    status = acs_mount(s, lock_id, vol_id, drive_id, readonly, bypass);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_mount() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE | READONLY | BYPASS, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            mp = (ACS_MOUNT_RESPONSE *) rbuf;

            if (mp->mount_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE mount_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(mp->mount_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_vol_info(0, 
                                     &(mp->mount_status), 
                                     &(mp->vol_id));

                    st_show_drv_info(0, 
                                     NULL, 
                                     &(mp->drive_id));
                }
                else
                {
                    st_chk_vol_info(0, 
                                    &(mp->mount_status), 
                                    STATUS_SUCCESS, 
                                    &(mp->vol_id), 
                                    &(vol_id));

                    st_chk_drv_info(0, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(mp->drive_id), 
                                    &drive_id);
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_mount_pinfo"
/*********************************************************************/
/* Input integer LOCKID or 0 or NO_LOCK_ID to ignore, plus volser to */
/* mount (if specific request), plus integer scratch subpool id      */
/* (used if not a specific request), plus management class name      */
/* string, plus media type (used if not a specific request), plus    */
/* BOOLEAN value indicating whether request is a scratch or          */
/* specific request, plus BOOLEAN value to                           */
/* indicate whether volume is mounted read-only, plus BOOLEAN value  */
/* to indicate whether volume is unlabelled (i.e. bypass volume      */
/* label checking for real tape cartridges with unreadable label),   */
/* plus jobname, plus stepname, plus dsn name, plus driveId to       */
/* mount.                                                            */
/* NOTE: If invoked under the XAPI, then the BOOLEAN bypass flag     */
/* is ignored.                                                       */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_mount_pinfo(int         lockId, 
                   char       *volIdString, 
                   int         poolId, 
                   char       *mgmtClasString, 
                   MEDIA_TYPE  mediaType, 
                   BOOLEAN     scratch, 
                   BOOLEAN     readonly, 
                   BOOLEAN     bypass, 
                   char       *jobnameString, 
                   char       *stepnameString, 
                   char       *dsnString, 
                   char        driveId[15])
{
    LOCKID              lock_id = (LOCKID) lockId;
    POOLID              pool_id;
    DRIVEID             drive_id;
    VOLID               vol_id;
    MGMT_CLAS           mgmt_clas;
    JOB_NAME            job_name;
    DATASET_NAME        dataset_name;
    STEP_NAME           step_name;
    int                 wkInt;
    char                driveIdSubstring[4];
    char                tbuf[EXTERNAL_LABEL_SIZE + 1];
    ACS_MOUNT_PINFO_RESPONSE *mp;

    /*****************************************************************/
    /* Input driveId is in format ACS:LSM:PAN:DRV (with each element */
    /* being 3 digits, separated by ":").                            */
    /* You can specify a virtual drive if you know the dummy         */
    /* ACS:LSM:PAN:DRV specification.                                */
    /*****************************************************************/
    memset(driveIdSubstring, 0, sizeof(driveIdSubstring));
    memcpy(driveIdSubstring, &(driveId[0]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.acs = (ACS) wkInt;

    memcpy(driveIdSubstring, &(driveId[4]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(driveIdSubstring, &(driveId[8]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.panel = (PANEL) wkInt;

    memcpy(driveIdSubstring, &(driveId[12]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.drive = (DRIVE) wkInt;

    pool_id.pool = (POOL) poolId;
    strcpy(vol_id.external_label, volIdString);
    strcpy(mgmt_clas.mgmt_clas, mgmtClasString);
    strcpy(job_name.job_name, jobnameString);
    strcpy(dataset_name.dataset_name, dsnString);
    strcpy(step_name.step_name, stepnameString);
    s = (SEQ_NO)450;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_mount_pinfo(s, lock_id, vol_id, pool_id, mgmt_clas, 
                             mediaType, scratch, readonly, bypass, 
                             job_name, dataset_name, step_name, drive_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_mount_pinfo() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            mp = (ACS_MOUNT_PINFO_RESPONSE *) rbuf;

            if (mp->mount_pinfo_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE mount_pinfo_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status IS %s \n", SELF, acs_status(mp->mount_pinfo_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_pool_info(0, 
                                      NULL, 
                                      &(mp->pool_id.pool));

                    st_show_drv_info(0, 
                                     NULL, 
                                     &(mp->drive_id));

                    st_show_vol_info(0, 
                                     &(mp->mount_pinfo_status), 
                                     &(mp->vol_id));
                }
                else
                {
                    st_chk_vol_info(0, 
                                    &(mp->mount_pinfo_status), 
                                    STATUS_SUCCESS, 
                                    &(mp->vol_id), 
                                    &(vol_id));

                    st_chk_drv_info(0, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(mp->drive_id), 
                                    &(drive_id));
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_mount_scratch"
/*********************************************************************/
/* Input integer LOCKID or 0 or NO_LOCK_ID to ignore, plus           */
/* integer scratch subpool id, plus media type, plus driveId to      */
/* mount scratch.                                                    */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_mount_scratch(int        lockId, 
                     int        poolId, 
                     MEDIA_TYPE mediaType, 
                     char       driveId[15])
{
    LOCKID              lock_id = (LOCKID) lockId;
    POOL                pool = (POOL) poolId;
    DRIVEID             drive_id;
    VOLID               vol_id;
    int                 wkInt;
    char                driveIdSubstring[4];
    char                tbuf[EXTERNAL_LABEL_SIZE + 1];
    ACS_MOUNT_SCRATCH_RESPONSE *mp;

    /*****************************************************************/
    /* Input driveId is in format ACS:LSM:PAN:DRV (with each element */
    /* being 3 digits, separated by ":").                            */
    /* You can specify a virtual drive if you know the dummy         */
    /* ACS:LSM:PAN:DRV specification.                                */
    /*****************************************************************/
    memset(driveIdSubstring, 0, sizeof(driveIdSubstring));
    memcpy(driveIdSubstring, &(driveId[0]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.acs = (ACS) wkInt;

    memcpy(driveIdSubstring, &(driveId[4]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(driveIdSubstring, &(driveId[8]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.panel_id.panel = (PANEL) wkInt;

    memcpy(driveIdSubstring, &(driveId[12]), 3);
    wkInt = atoi(driveIdSubstring);
    drive_id.drive = (DRIVE) wkInt;

    s = (SEQ_NO)302;

    /* If not a simulated server, then skip this request */
    if ((real_server) &&
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_mount_scratch(s, lock_id, pool, drive_id, mediaType);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_mount_scratch() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            mp = (ACS_MOUNT_SCRATCH_RESPONSE *) rbuf;

            if (mp->mount_scratch_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE mount_scratch_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(mp->mount_scratch_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_pool_info(0, 
                                      NULL, 
                                      &(mp->pool));

                    st_show_vol_info(0, 
                                     NULL, 
                                     &(mp->vol_id));

                    st_show_drv_info(0, 
                                     NULL, 
                                     &(mp->drive_id));
                }
                else
                {
                    st_chk_pool_info(0, 
                                     &(mp->pool), 
                                     NULL, 
                                     &pool, 
                                     STATUS_SUCCESS);

                    sprintf(tbuf, "PB9999");
                    strcpy(&(vol_id.external_label[0]), &(tbuf[0]));

                    st_chk_vol_info(0, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(mp->vol_id), 
                                    &(vol_id));

                    st_chk_drv_info(0, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(mp->drive_id), 
                                    &(drive_id));
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_acs"
/*********************************************************************/
/* Input count of acsId(s) to query, plus strings for each acsId.    */
/*                                                                   */
/* Format of input acsId(s) is:                                      */
/* "decimal ACS" with ACS being 3 digit string.                      */
/* Example: "127" (ACS=127)                                          */
/*********************************************************************/
void c_query_acs(int count, ...)
{
    ACS                 acs[MAX_ID];
    char               *pAcsIdString;
    char                acsIdString[4];
    int                 wkInt;
    int                 i;
    va_list             acsIds;
    QU_ACS_STATUS      *sp;
    ACS_QUERY_ACS_RESPONSE *qp;

    va_start(acsIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pAcsIdString = va_arg(acsIds, char*);
        memset(acsIdString, 0 , sizeof(acsIdString));
        memcpy(acsIdString, &(pAcsIdString[0]), 3);
        wkInt = atoi(acsIdString);
        acs[i] = (ACS) wkInt;
    }

    va_end(acsIds);

    s = (SEQ_NO)501;
    status = acs_query_acs(s, acs, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_acs() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_ACS_RESPONSE *) rbuf;

            if (qp->query_acs_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_acs_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_acs_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->acs_status[i]);

                        st_show_acs_status(i, 
                                           sp);
                    }
                }
                else
                {
                    st_chk_acs_status(count, 
                                      qp, 
                                      &(acs[0]));
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_cap"
/*********************************************************************/
/* Input count of capId(s) to query, plus strings for each capId.    */
/*                                                                   */
/* Format of input capId(s) is:                                      */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*********************************************************************/
void c_query_cap(int count, ...)
{
    CAPID               cap_id[MAX_ID];
    char               *pCapIdString;
    char                capIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             capIds;
    QU_CAP_STATUS      *sp;
    ACS_QUERY_CAP_RESPONSE *qp;

    va_start(capIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pCapIdString = va_arg(capIds, char*);
        memset(capIdSubstring, 0 , sizeof(capIdSubstring));
        memcpy(capIdSubstring, &(pCapIdString[0]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.acs = (ACS) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[4]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.lsm = (LSM) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[8]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].cap = (CAP) wkInt;
    }

    va_end(capIds);

    s = (SEQ_NO)502;
    status = acs_query_cap(s, cap_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_cap() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_CAP_RESPONSE *) rbuf;

            if (qp->query_cap_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_cap_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_cap_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &qp->cap_status[i];

                        st_show_cap_info(i, 
                                         NULL, 
                                         &(sp->cap_id));

                        st_show_cap_misc(i, 
                                         &(sp->cap_state), 
                                         sp->cap_mode, 
                                         sp->cap_priority);

                        CDR_MSG(4, "\t%s: cap[%d] cap_size = %d\n", SELF, i, sp->cap_size);
                    }
                }
                else
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->cap_status[i]);

                        if (count)
                        {
                            st_chk_cap_info(i, 
                                            i, 
                                            NULL, 
                                            STATUS_SUCCESS, 
                                            &(sp->cap_id), 
                                            &(cap_id[i]));
                        }
                        else
                        {
                            cap_id[0].lsm_id.acs = i;
                            cap_id[0].lsm_id.lsm = i + 1;
                            cap_id[0].cap = i + 2;

                            st_chk_cap_info(i, 
                                            i, 
                                            NULL, 
                                            STATUS_SUCCESS, 
                                            &(sp->cap_id), 
                                            &(cap_id[0]));
                        }

                        st_chk_cap_misc(i, 
                                        &(sp->cap_state), 
                                        STATE_ONLINE, 
                                        sp->cap_mode, 
                                        CAP_MODE_AUTOMATIC, 
                                        sp->cap_priority, 
                                        (CAP_PRIORITY) 9);

                        if (sp->cap_state != STATE_ONLINE)
                        {
                            CDR_MSG(0, "\t%s: cap[%d] state failure\n", SELF, i);
                            CDR_MSG(0, "\t    expected %s, got %s\n", 
                                    acs_state(STATE_ONLINE), 
                                    acs_state(sp->cap_state));
                        }

                        if (sp->cap_size != (unsigned short)100)
                        {
                            CDR_MSG(0, "\t%s: cap[%d] size failure\n", SELF, i);
                            CDR_MSG(0, "\t    expected %d, got %d\n", 
                                    (unsigned short)100, sp->cap_size);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_clean"
void c_query_clean()
{
    int                 i;
    VOLID               vol_id[MAX_ID];
    ACS_QUERY_CLN_RESPONSE *qp;
    QU_CLN_STATUS      *sp;
    int                 count;
    QU_CLN_STATUS       clean_stat;

    s = (SEQ_NO)502;

    if (!check_mode)
    {
        count = 0;
    }
    else
    {
        count = 1;
    }

    strncpy(vol_id[0].external_label, "PB3333", EXTERNAL_LABEL_SIZE);
    status = acs_query_clean(s, vol_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_clean() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_CLN_RESPONSE *) rbuf;

            if (qp->query_cln_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_cln_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_cln_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->cln_status[i]);

                        st_show_vol_info(i, 
                                         &(sp->status), 
                                         &(sp->vol_id));

                        st_show_cellid(i, 
                                       NULL, 
                                       &(sp->home_location));

                        st_show_media_type(i, 
                                           sp->media_type);

                        st_show_usage(sp);
                    }
                }
                else
                {
                    clean_stat.status = STATUS_VOLUME_HOME;
                    clean_stat.media_type = ANY_MEDIA_TYPE;

                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->cln_status[i]);

                        if (count)
                        {
                            st_chk_vol_info(i, 
                                            &(sp->status), 
                                            STATUS_VOLUME_HOME, 
                                            &(sp->vol_id), 
                                            &(vol_id[i]));
                        }
                        else
                        {
                            sprintf(&(vol_id[0].external_label[0]), "PB03%02d", i);

                            st_chk_vol_info(i, 
                                            &(sp->status), 
                                            STATUS_VOLUME_HOME, 
                                            &(sp->vol_id), 
                                            &(vol_id[0]));
                        }

                        clean_stat.home_location.panel_id.lsm_id.acs = (ACS) i;
                        clean_stat.home_location.panel_id.lsm_id.lsm = (LSM) i + 1;
                        clean_stat.home_location.panel_id.panel = (PANEL) i + 2;
                        clean_stat.home_location.row = (ROW) i + 3;
                        clean_stat.home_location.col = (COL) i + 4;
                        clean_stat.max_use = (unsigned short)1000 + i;
                        clean_stat.current_use = (unsigned short)9 + i;

                        st_chk_cellid(i, 
                                      &(sp->home_location), 
                                      &(clean_stat.home_location));

                        st_chk_media_type(i, 
                                          sp->media_type, 
                                          clean_stat.media_type);

                        st_chk_usage(i, 
                                     sp, 
                                     &clean_stat);
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_query_drive"
/*********************************************************************/
/* Input count of driveId(s) to query, plus strings for each driveId.*/
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_query_drive(int count, ...)
{
    DRIVEID             drive_id[MAX_ID];
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    QU_DRV_STATUS      *sp;
    QU_DRV_STATUS       drv_stat;
    ACS_QUERY_DRV_RESPONSE *qp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);

    s = (SEQ_NO)503;
    status = acs_query_drive(s, drive_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_drive() failed %s\n", SELF, acs_status(status));
        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_DRV_RESPONSE *) rbuf;

            if (qp->query_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_drv_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->drv_status[i]);

                        st_show_drv_status(i, 
                                           sp);
                    }
                }
                else
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &(qp->drv_status[i]);

                        if (count)
                        {
                            drv_stat.drive_id = drive_id[i];
                        }
                        else
                        {
                            drive_id[0].panel_id.lsm_id.acs = i;
                            drive_id[0].panel_id.lsm_id.lsm = i + 1;
                            drive_id[0].panel_id.panel = 10;
                            drive_id[0].drive = 3;
                            drv_stat.drive_id = drive_id[0];
                        }

                        sprintf(drv_stat.vol_id.external_label, "PB03%02d", i);
                        drv_stat.state = STATE_ONLINE;
                        drv_stat.status = STATUS_DRIVE_IN_USE;
                        drv_stat.drive_type = i + 4;

                        st_chk_drv_status(i, 
                                          sp, 
                                          &drv_stat);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_drive_group"
/*********************************************************************/
/* Input count of drive groups(s) to query, plus strings for each    */
/* drive group (or VTSS name).                                       */
/*                                                                   */
/* Format of input groupId(s) is:                                    */
/* "vtssname" with vtssname being 8 character string.                */
/* Example: "SVTSS1" (VTSS=SVTSS1)                                   */
/*********************************************************************/
void c_query_drive_group(int count, ...)
{
    GROUPID             group_id[MAX_DRG];
    GROUP_TYPE          group_type;
    char               *pGroupIdString;
    char                groupIdString[9];
    int                 wkInt;
    int                 i;
    int                 j;
    va_list             groupIds;

    QU_VIRT_DRV_MAP    *sp;
    ACS_QU_DRV_GROUP_RESPONSE *qp;

    va_start(groupIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pGroupIdString = va_arg(groupIds, char*);
        memset(groupIdString, 0 , sizeof(groupIdString));
        strncpy(groupIdString, pGroupIdString, 8);

        for (j = 0;
            j < 8;
            j++)
        {
            if (groupIdString[j] == 0)
            {
                groupIdString[j] = ' ';
            }
        }

        strcpy(group_id[i].groupid, groupIdString);
    }

    va_end(groupIds);

    if ((!(libstation)) &&
        (!(xapi)))
    {
        CDR_MSG(0, "This command NOT supported by ACSLS\n");
    }

    s = (SEQ_NO)511;
    group_type=GROUP_TYPE_VTSS;

    status = acs_query_drive_group(s, group_type, count, group_id);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_drive_group() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QU_DRV_GROUP_RESPONSE *) rbuf;

            if (qp->query_drv_group_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_drv_group_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_drv_group_status));
            }
            else
            {
                if (!check_mode)
                {
                    CDR_MSG(4, "\t%s: group_id is %s\n", SELF, &(qp->group_id));
                    CDR_MSG(4, "\t%s: group_type is %d\n", SELF, qp->group_type);
                    CDR_MSG(4, "\t%s: drive_group map count is %d\n", SELF, qp->count);

                    for (i = 0; i < qp->count; i++)
                    {
                        st_show_drv_info(i, 
                                         NULL, 
                                         &(qp->virt_drv_map[i].drive_id));

                        CDR_MSG(4, "\t%s: drive_addr is %d\n", SELF, 
                                qp->virt_drv_map[i].drive_addr);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_lock_drive"
/*********************************************************************/
/* Input integer LOCKID, plus integer count of driveId(s) to query,  */
/* plus driveId string(s).                                           */
/* NOTE: LOCKID value of 0 or NO_LOCK_ID should return an error.     */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_query_lock_drive(int lockId, 
                        int count, ...)
{
    DRIVEID             drive_id[MAX_ID];
    LOCKID              lock_id = (LOCKID) lockId;
    USERID              u_id;
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    QL_DRV_STATUS      *sp;
    ACS_QUERY_LOCK_DRV_RESPONSE *qp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);

    s = (SEQ_NO)509;
    status = acs_query_lock_drive(s, drive_id, lock_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_lock_drive() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            memcpy((char*) &wkInt, (char*) rbuf, 4);

            TRMEM(rbuf, wkInt, "QUERY_DRIVE LOCK:\n:");

            qp = (ACS_QUERY_LOCK_DRV_RESPONSE *) rbuf;

            if (qp->query_lock_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_lock_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_lock_drv_status));
            }
            else
            {
                for (i = 0; i < (int) qp->count; i++)
                {
                    sp = &(qp->drv_status[i]);

                    if (!check_mode)
                    {
                        st_show_drv_info(i, 
                                         &(sp->status), 
                                         &(sp->drive_id));

                        st_show_lock_info(i, 
                                          sp->lock_id, 
                                          sp->lock_duration, 
                                          sp->locks_pending, 
                                          &(sp->user_id));
                    }
                    else
                    {
                        sprintf(u_id.user_label, "Ingemar");

                        st_chk_drv_info(i, 
                                        &(sp->status), 
                                        STATUS_DRIVE_AVAILABLE, 
                                        &(sp->drive_id), 
                                        &drive_id[i]);

                        st_chk_lock_info(i, 
                                         sp->lock_id, 
                                         99, 
                                         sp->lock_duration, 
                                         9999, 
                                         sp->locks_pending, 
                                         9, 
                                         &(sp->user_id), 
                                         &u_id);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_lock_volume"
/*********************************************************************/
/* Input integer LOCKID, plus integer count of volser(s) to query,   */
/* plus volser string(s).                                            */
/* NOTE: LOCKID value of 0 or NO_LOCK_ID should return an error.     */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_query_lock_volume(int lockId, 
                         int count, ...)
{
    VOLID               vol_id[MAX_ID];
    LOCKID              lock_id = (LOCKID) lockId;
    USERID              u_id;
    char               *pVolIdString;
    int                 i;
    va_list             volIds;
    QL_VOL_STATUS      *sp;
    ACS_QUERY_LOCK_VOL_RESPONSE *qp;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO)509;
    status = acs_query_lock_volume(s, vol_id, lock_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_lock_volume() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_LOCK_VOL_RESPONSE *) rbuf;

            if (qp->query_lock_vol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_lock_vol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_lock_vol_status));
            }
            else
            {
                for (i = 0; i < qp->count; i++)
                {
                    sp = &(qp->vol_status[i]);

                    if (!check_mode)
                    {
                        st_show_vol_info(i, 
                                         &(sp->status), 
                                         &(sp->vol_id));

                        st_show_lock_info(i, 
                                          sp->lock_id, 
                                          sp->lock_duration, 
                                          sp->locks_pending, 
                                          &(sp->user_id));
                    }
                    else
                    {
                        sprintf(u_id.user_label, "Pirmin");

                        st_chk_vol_info(i, 
                                        &(sp->status), 
                                        STATUS_VOLUME_AVAILABLE, 
                                        &(sp->vol_id), 
                                        &(vol_id[i]));

                        st_chk_lock_info(i,
                                         sp->lock_id, 
                                         99, 
                                         sp->lock_duration, 
                                         9999, 
                                         sp->locks_pending, 
                                         9, 
                                         &(sp->user_id), 
                                         &u_id);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_lsm"
/*********************************************************************/
/* Input count of lsmId(s) to query, plus strings for each lsmId.    */
/*                                                                   */
/* Format of input lsmid(s) is:                                      */
/* "decimal ACS:decimal LSM"                                         */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002" (ACS=001, LSM=002)                             */
/*********************************************************************/
void c_query_lsm(int count, ...)
{
    LSMID               lsm_id[MAX_ID];
    int                 i;
    int                 j;
    int                 k;
    char               *pLsmIdString;
    char                lsmIdSubstring[4];
    int                 wkInt;
    va_list             lsmIds;
    REQ_SUMMARY         reqs;
    QU_LSM_STATUS      *sp;
    ACS_QUERY_LSM_RESPONSE *qp;

    va_start(lsmIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pLsmIdString = va_arg(lsmIds, char*);
        memset(lsmIdSubstring, 0 , sizeof(lsmIdSubstring));
        memcpy(lsmIdSubstring, &(pLsmIdString[0]), 3);
        wkInt = atoi(lsmIdSubstring);
        lsm_id[i].acs = (ACS) wkInt;

        memcpy(lsmIdSubstring, &(pLsmIdString[4]), 3);
        wkInt = atoi(lsmIdSubstring);
        lsm_id[i].lsm = (LSM) wkInt;
    }

    va_end(lsmIds);

    s = (SEQ_NO)504;
    status = acs_query_lsm(s, lsm_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_lsm() failed %s\n", SELF, acs_status(status));
        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_LSM_RESPONSE *) rbuf;

            if (qp->query_lsm_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_lsm_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_lsm_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &qp->lsm_status[i];

                        st_show_lsm_id(i, 
                                       &(sp->status), 
                                       &(sp->lsm_id));

                        st_show_state(i, 
                                      sp->state);

                        st_show_freecells(i, 
                                          sp->freecells);

                        st_show_requests(i, 
                                         &(sp->requests));
                    }
                }
                else
                {
                    for (j = k = 0; k < 5; k++, j++)
                    {
                        reqs.requests[k][0] = j;
                        reqs.requests[k][1] = j + 1;
                    }

                    for (i = 0; i < (int) qp->count; i++)
                    {
                        sp = &qp->lsm_status[i];

                        if (count)
                        {
                            st_chk_lsm_id(i, 
                                          &(sp->status), 
                                          STATUS_CAP_AVAILABLE, 
                                          &(sp->lsm_id), 
                                          &(lsm_id[i]));
                        }
                        else
                        {
                            lsm_id[0].acs = i;
                            lsm_id[0].lsm = i + 1;

                            st_chk_lsm_id(i, 
                                          &(sp->status), 
                                          STATUS_CAP_AVAILABLE, 
                                          &(sp->lsm_id), 
                                          &(lsm_id[0]));
                        }

                        st_chk_state(i, 
                                     sp->state, 
                                     STATE_OFFLINE_PENDING);

                        st_chk_freecells(i, 
                                         sp->freecells, 
                                         100);

                        st_chk_requests(&(sp->requests), 
                                        &reqs);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_mm_info"
/*********************************************************************/
/* No input; always return ALL drive type and media type info.       */
/*********************************************************************/
void c_query_mm_info()
{
    int                 j;
    unsigned short      m_count;
    unsigned short      d_count;
    QU_MMI_RESPONSE    *sp;
    QU_DRIVE_TYPE_STATUS *dip;
    QU_MEDIA_TYPE_STATUS *mip;
    QU_DRIVE_TYPE_STATUS *dp;
    QU_MEDIA_TYPE_STATUS *mp;
    ACS_QUERY_MMI_RESPONSE *qp;
    QU_MEDIA_TYPE_STATUS m_info[MAX_ID];
    QU_DRIVE_TYPE_STATUS d_info[MAX_ID];

    m_count = 2;
    m_info[0].media_type = 1;
    m_info[1].media_type = 4;
    strcpy(m_info[0].media_type_name, "media_1");
    strcpy(m_info[1].media_type_name, "media_4");
    m_info[0].cleaning_cartridge = CLN_CART_INDETERMINATE;
    m_info[1].cleaning_cartridge = CLN_CART_NEVER;
    m_info[0].max_cleaning_usage = 13;
    m_info[1].max_cleaning_usage = 29;
    m_info[0].compat_count = 1;
    m_info[1].compat_count = 2;
    m_info[0].compat_drive_types[0] = 2;
    m_info[1].compat_drive_types[0] = 2;
    m_info[1].compat_drive_types[1] = 3;
    d_count = 2;
    d_info[0].drive_type = 2;
    d_info[1].drive_type = 3;
    strcpy(d_info[0].drive_type_name, "drive_2");
    strcpy(d_info[1].drive_type_name, "drive_3");
    d_info[0].compat_count = 2;
    d_info[1].compat_count = 1;
    d_info[0].compat_media_types[0] = 1;
    d_info[0].compat_media_types[1] = 4;
    d_info[1].compat_media_types[0] = 4;

    s = (SEQ_NO)555;
    status = acs_query_mm_info(s);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_mm_info() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (status == STATUS_INVALID_TYPE)
        {
            if (acs_get_packet_version() >= 4)
            {
                CDR_MSG(0, "\t\tacs_response: status = STATUS_INVALID_TYPE\n");
            }

            break;
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_MMI_RESPONSE *) rbuf;

            if (qp->query_mmi_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_mmi_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_mmi_status));
            }
            else
            {
                sp = &qp->mixed_media_info_status;

                for (j = 0; j < (int) sp->media_type_count; j++)
                {
                    mip = &sp->media_type_status[j];
                    mp = &m_info[j];

                    if (!check_mode)
                    {
                        st_show_media_info(j, 
                                           mip);
                    }
                    else
                    {
                        if (sp->media_type_count != m_count && (j == 0))
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE media_count failure\n", SELF);
                            CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                                    SELF, sp->media_type_count, m_count);
                        }

                        st_chk_media_info(j, 
                                          mip, 
                                          mp);
                    }
                }

                for (j = 0; j < (int) sp->drive_type_count; j++)
                {
                    dip = &sp->drive_type_status[j];
                    dp = &d_info[j];

                    if (!check_mode)
                    {
                        st_show_drive_type_info(j, 
                                                dip);
                    }
                    else
                    {
                        if (sp->drive_type_count != d_count)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE drive_count failure\n", SELF);
                            CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                                    SELF, sp->drive_type_count, d_count);
                        }

                        st_chk_drive_type_info(j, 
                                               dip, 
                                               dp);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_mount"
/*********************************************************************/
/* Input count of volser(s) to query, plus strings for each volser.  */
/*                                                                   */
/* Format of input volser(s) is:                                     */
/* "VVVVVV" with each volser being 6 characters.                     */
/* Example: "TIM021,S00023"                                          */
/*********************************************************************/
void c_query_mount(int count, ...)
{
    unsigned short      i;
    unsigned short      j;
    VOLID               vol_id[MAX_ID];
    char               *pVolIdString;
    va_list             volIds;
    QU_MNT_STATUS      *sp;
    QU_DRV_STATUS      *dp;
    ACS_QUERY_MNT_RESPONSE *qp;
    QU_DRV_STATUS       drv_stat;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO)505;
    status = acs_query_mount(s, vol_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_mount() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_MNT_RESPONSE *) rbuf;

            if (qp->query_mnt_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_mnt_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_mnt_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->mnt_status[i];

                        st_show_vol_info(i, 
                                         &(sp->status), 
                                         &(sp->vol_id));

                        for (j = 0; j < sp->drive_count; j++)
                        {
                            dp = &sp->drive_status[j];

                            st_show_drv_status(j, 
                                               dp);
                        }
                    }
                }
                else
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->mnt_status[i];

                        st_chk_vol_info(i, 
                                        &(sp->status), 
                                        STATUS_VOLUME_HOME, 
                                        &(sp->vol_id), 
                                        &(vol_id[i]));

                        for (j = 0; j < sp->drive_count; j++)
                        {
                            dp = &sp->drive_status[j];
                            drv_stat.drive_id.panel_id.lsm_id.acs = j;
                            drv_stat.drive_id.panel_id.lsm_id.lsm = j + 1;
                            drv_stat.drive_id.panel_id.panel = j + 2;
                            drv_stat.drive_id.drive = j + 3;
                            drv_stat.drive_type = j + 4;
                            sprintf(drv_stat.vol_id.external_label, "PB04%02d", j);
                            drv_stat.state = STATE_OFFLINE;
                            drv_stat.status = STATUS_DRIVE_IN_USE;

                            st_chk_drv_status(j, 
                                              dp, 
                                              &drv_stat);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_mount_scratch"
/*********************************************************************/
/* Input MEDIA_TYPE of volsers, plus count of scratch subpools,      */
/* plus integers representing each scratch subpool index.            */
/*                                                                   */
/* Example: ANY_MEDIA_TYPE, 2, 10, 15                                */
/* to return eligible drives for any media for pools 10 and 15.      */
/*********************************************************************/
void c_query_mount_scratch(MEDIA_TYPE mediaType, 
                           int        count, ...)
{
    unsigned short      i;
    unsigned short      j;
    POOL                pool[MAX_ID];
    va_list             poolIds;
    ACS_QUERY_MSC_RESPONSE *qp;
    QU_MSC_STATUS      *sp;
    QU_DRV_STATUS      *dp;
    QU_DRV_STATUS       drv_stat;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)505;
    status = acs_query_mount_scratch(s, pool, count, mediaType);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_mount_scratch() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_MSC_RESPONSE *) rbuf;

            if (qp->query_msc_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_msc_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_msc_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->msc_status[i];

                        st_show_pool_info(i, 
                                          &(sp->status), 
                                          &(sp->pool_id.pool));

                        if (sp->drive_count > 0)
                        {
                            CDR_MSG(4, "\t%s: drive list for pool[%d] id %d contains %d drives:\n", 
                                    SELF, i, sp->pool_id.pool, sp->drive_count);

                            for (j = 0; j < sp->drive_count; j++)
                            {
                                dp = &sp->drive_list[j];

                                st_show_drv_status(j, 
                                                   dp);
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->msc_status[i];

                        st_chk_pool_info(i, 
                                         &(sp->pool_id.pool), 
                                         &(sp->status),
                                         &(pool[i]), 
                                         STATUS_POOL_LOW_WATER);

                        for (j = 0; j < sp->drive_count; j++)
                        {
                            dp = &sp->drive_list[j];
                            drv_stat.drive_id.panel_id.lsm_id.acs = j;
                            drv_stat.drive_id.panel_id.lsm_id.lsm = j + 1;
                            drv_stat.drive_id.panel_id.panel = j + 2;
                            drv_stat.drive_id.drive = j + 3;
                            drv_stat.drive_type = j + 4;
                            sprintf(drv_stat.vol_id.external_label, "PB03%02d", j);
                            drv_stat.state = STATE_ONLINE;
                            drv_stat.status = STATUS_DRIVE_IN_USE;

                            st_chk_drv_status(j, 
                                              dp, 
                                              &drv_stat);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_mount_scratch_pinfo"
/*********************************************************************/
/* Input MEDIA_TYPE code, plus management class name, plus count     */
/* of subpools, plus integers to represent each scratch subpool      */
/* index.                                                            */
/*                                                                   */
/* Example: ANY_MEDIA_TYPE, "MGMTC001", 2, 10, 15                    */
/* to return eligible drives for MGMT_CLASS "MGMTC001", any media    */
/* for pools 10 and 15                                               */
/*********************************************************************/
void c_query_mount_scratch_pinfo(MEDIA_TYPE  mediaType, 
                                 char       *mgmtClasString, 
                                 int         count, ...)
{
    unsigned short      i;
    unsigned short      j;
    POOL                pool[MAX_ID];
    MGMT_CLAS           mgmt_clas;
    va_list             poolIds;
    ACS_QUERY_MSC_RESPONSE *qp;
    QU_MSC_STATUS      *sp;
    QU_DRV_STATUS      *dp;
    QU_DRV_STATUS       drv_stat;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)513;
    strcpy(mgmt_clas.mgmt_clas, mgmtClasString);
    status = acs_query_mount_scratch_pinfo(s, pool, count, mediaType, mgmt_clas);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_mount_scratch_pinfo() failed %s\n",
                SELF, acs_status(status)); 

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_MSC_RESPONSE *) rbuf;

            if (qp->query_msc_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_msc_pinfo_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_msc_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->msc_status[i];

                        st_show_pool_info(i, 
                                          &(sp->status), 
                                          &(sp->pool_id.pool));

                        if (sp->drive_count > 0)
                        {
                            CDR_MSG(4, "\t%s: drive list for pool[%d] id %d contains %d drives:\n", 
                                    SELF, i, sp->pool_id.pool, sp->drive_count);

                            for (j = 0; j < sp->drive_count; j++)
                            {
                                dp = &sp->drive_list[j];

                                st_show_drv_status(j, 
                                                   dp);
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->msc_status[i];

                        st_chk_pool_info(i, 
                                         &(sp->pool_id.pool), 
                                         &(sp->status),
                                         &(pool[i]), 
                                         STATUS_POOL_LOW_WATER);

                        for (j = 0; j < sp->drive_count; j++)
                        {
                            dp = &sp->drive_list[j];
                            drv_stat.drive_id.panel_id.lsm_id.acs = j;
                            drv_stat.drive_id.panel_id.lsm_id.lsm = j + 1;
                            drv_stat.drive_id.panel_id.panel = j + 2;
                            drv_stat.drive_id.drive = j + 3;
                            drv_stat.drive_type = j + 4;
                            sprintf(drv_stat.vol_id.external_label, "PB03%02d", j);
                            drv_stat.state = STATE_ONLINE;
                            drv_stat.status = STATUS_DRIVE_IN_USE;

                            st_chk_drv_status(j, 
                                              dp, 
                                              &drv_stat);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_pool"
/*********************************************************************/
/* Input count, plus number of integer pools to return, plus         */
/* integers representing each scratch subpool index.                 */
/*                                                                   */
/* If count = 0, then return all pools.                              */
/* Example: 2, 10, 15 to just return subpools 10 and 15.             */
/*********************************************************************/
void c_query_pool(int count, ...)
{
    int                 i;
    POOL                pool[MAX_ID];
    va_list             poolIds;
    ACS_QUERY_POL_RESPONSE *qp;
    QU_POL_STATUS      *sp;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)506;
    status = acs_query_pool(s, pool, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_pool() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_POL_RESPONSE *) rbuf;

            if (qp->query_pol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_pol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_pol_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->pool_status[i];

                        st_show_pool_info(i, 
                                          &(sp->status), 
                                          &(sp->pool_id.pool));

                        st_show_pool_misc(i,
                                          sp->low_water_mark, 
                                          sp->high_water_mark, 
                                          sp->pool_attributes);

                        CDR_MSG(4, "\t%s: volume count is %d\n", SELF, 
                                sp->volume_count);
                    }
                }
                else
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &qp->pool_status[i];

                        st_chk_pool_info(i, 
                                         &(sp->pool_id.pool), 
                                         &(sp->status), 
                                         &(pool[i]), 
                                         STATUS_POOL_HIGH_WATER);

                        st_chk_pool_misc(sp->low_water_mark, 
                                         20 + i, 
                                         sp->high_water_mark, 
                                         40 + i, 
                                         sp->pool_attributes, 
                                         OVERFLOW);

                        if (sp->volume_count != 62 + i)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE vol_count failure\n", SELF);
                            CDR_MSG(0, "\t%s: got %d, expected %d\n", SELF, 
                                    sp->volume_count, 62 + i);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_port"
void c_query_port()
{
    int                i;
    int                count;
    PORTID             port_id[MAX_ID];
    QU_PRT_STATUS     *sp;
    ACS_QUERY_PRT_RESPONSE *qp;

    s = (SEQ_NO)506;
    count = 0;

    if (!check_mode)
    {
        count = 0;
    }
    else
    {
        count = 1;
    }
    port_id[0].acs = 0;
    port_id[0].port = 0;

    status = acs_query_port(s, port_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_port() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_PRT_RESPONSE *) rbuf;

            if (qp->query_prt_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_port_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_prt_status));
            }

            if ((count != 0) && (qp->count != count))
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE count failure\n", SELF);
            }
            else
            {
                for (i = 0; i < (int) qp->count; i++)
                {
                    sp = &qp->prt_status[i];

                    if (!check_mode)
                    {
                        st_show_port_info(i, 
                                          &sp->port_id, 
                                          sp->state, 
                                          sp->status);
                    }
                    else
                    {
                        st_chk_port_info(i, 
                                         &sp->port_id, 
                                         &port_id[i], 
                                         sp->state, 
                                         STATE_OFFLINE_PENDING, 
                                         sp->status, 
                                         STATUS_SUCCESS);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_request"
/*********************************************************************/
/* Input count or requests, plus integer SEQ_NO or packet ids to     */
/* return.                                                           */
/*                                                                   */
/* If count = 0, then return all active requests.                    */
/*********************************************************************/
void c_query_request(int count, ...)
{
    int                 i;
    REQ_ID              request_ids[MAX_ID];
    va_list             reqIds;

    QU_REQ_STATUS      *sp;
    QU_REQ_STATUS       req_stat;
    ACS_QUERY_REQ_RESPONSE *qp;

    va_start(reqIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        request_ids[i] = (REQ_ID) va_arg(reqIds, int);
    }

    va_end(reqIds);

    s = (SEQ_NO)507;
    status = acs_query_request(s, request_ids, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_request() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_REQ_RESPONSE *) rbuf;

            if (qp->query_req_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_req_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_req_status));
            }

            if ((count != 0) && (qp->count != count))
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE count failure\n", SELF);
            }
            else
            {
                req_stat.command = COMMAND_DISMOUNT;
                req_stat.status = STATUS_PENDING;

                for (i = 0; i < (int) qp->count; i++)
                {
                    sp = &qp->req_status[i];

                    if (!check_mode)
                    {
                        st_show_req_status(i, 
                                           sp);
                    }
                    else
                    {
                        req_stat.request = i;

                        st_chk_req_status(i, 
                                          sp, 
                                          &req_stat);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_scratch"
/*********************************************************************/
/* Input count of scratch subpools to return, plus integers          */
/* representing each scratch subpool index desired.                  */
/*                                                                   */
/* If count = 0, then return all pools.                              */
/*********************************************************************/
void c_query_scratch(int count, ...)
{
    int                 i;
    POOL                pool[MAX_ID];
    va_list             poolIds;
    QU_SCR_STATUS      *sp;
    VOLID               vol_id;
    CELLID              cell_id;
    ACS_QUERY_SCR_RESPONSE *qp;

    va_start(poolIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pool[i] = (POOL) va_arg(poolIds, int);
    }

    va_end(poolIds);

    s = (SEQ_NO)508;
    status = acs_query_scratch(s, pool, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_scratch() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }
            qp = (ACS_QUERY_SCR_RESPONSE *) rbuf;

            if (qp->query_scr_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_scr_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_scr_status));
            }
            else
            {
                for (i = 0; i < (int) qp->count; i++)
                {
                    sp = &qp->scr_status[i];

                    if (!check_mode)
                    {
                        st_show_vol_info(i, 
                                         &(sp->status), 
                                         &sp->vol_id);

                        st_show_cellid(i, 
                                       NULL, 
                                       &sp->home_location);

                        st_show_media_type(i, 
                                           sp->media_type);

                        st_show_pool_info(i, 
                                          NULL, 
                                          &(sp->pool_id.pool));
                    }
                    else
                    {
                        sprintf(vol_id.external_label, "PB03%02d", i);

                        st_chk_vol_info(i, 
                                        &(sp->status), 
                                        STATUS_VOLUME_HOME, 
                                        & sp->vol_id, 
                                        &vol_id);

                        cell_id.panel_id.lsm_id.acs = i;
                        cell_id.panel_id.lsm_id.lsm = i + 1;
                        cell_id.panel_id.panel = i + 2;
                        cell_id.row = i + 3;
                        cell_id.col = i + 4;

                        st_chk_cellid(i, 
                                      &(sp->home_location), 
                                      &(cell_id));

                        st_chk_media_type(i, 
                                          sp->media_type, 
                                          i + 5);

                        st_chk_pool_info(i, 
                                         &sp->pool_id.pool, 
                                         NULL, 
                                         &pool[i], 
                                         STATUS_SUCCESS);
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_server"
void c_query_server()
{
    int                 j;
    int                 k;
    QU_SRV_STATUS      *sp;
    ACS_QUERY_SRV_RESPONSE *qp;
    REQ_SUMMARY         reqs;

    s = (SEQ_NO)508;
    status = acs_query_server(s);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_server() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_SRV_RESPONSE *) rbuf;

            if (qp->query_srv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_srv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_srv_status));
            }

            if (qp->count != 1)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE count failure\n", SELF);
            }
            else
            {
                sp = &qp->srv_status[0];

                if (!check_mode)
                {
                    st_show_state(0, 
                                  sp->state);

                    st_show_freecells(0, 
                                      sp->freecells);

                    st_show_requests(0, 
                                     &(sp->requests));
                }
                else
                {
                    for (j = k = 0; k < 5; k++, j++)
                    {
                        reqs.requests[k][0] = j;
                        reqs.requests[k][1] = j + 1;
                    }

                    st_chk_freecells(0, 
                                     sp->freecells, 
                                     100);

                    st_chk_state(0, 
                                 sp->state, 
                                 STATE_RUN);

                    st_chk_requests(&(sp->requests), 
                                    &reqs);
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_subpool_name"
/*********************************************************************/
/* Input count of scratch subpools to return, plus subpool           */
/* name strings for each scratch subpool desired.                    */
/*                                                                   */
/* Format of subpool name is 13 alpha-numeric characters (max).      */
/* Example: "ACCTCARTS" (subpool name=ACCTCARTS)                     */
/*********************************************************************/
void c_query_subpool_name(int count, ...)
{
    SUBPOOL_NAME        subpool_name[MAX_SPN];
    char               *pSubpoolNameString;
    int                 i;
    ACS_QU_SUBPOOL_NAME_RESPONSE *qp;
    QU_SUBPOOL_NAME_STATUS       *sp;

    va_list             subpoolNames;

    memset(subpool_name, 0 , sizeof(subpool_name));
    va_start(subpoolNames, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pSubpoolNameString = va_arg(subpoolNames, char*);
        strcpy(subpool_name[i].subpool_name, pSubpoolNameString);
    }

    va_end(subpoolNames);

    s = (SEQ_NO)509;

    status = acs_query_subpool_name(s, count, subpool_name);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_subpool_name() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QU_SUBPOOL_NAME_RESPONSE *) rbuf;

            if (qp->query_subpool_name_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_subpool_name_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", 
                        SELF, acs_status(qp->query_subpool_name_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < qp->count; i++)
                    {
                        sp = &(qp->subpool_name_status[i]);

                        CDR_MSG(4, "\t%s: subpool name is %s\n", SELF,
                                &(qp->subpool_name_status[i].subpool_name));

                        CDR_MSG(4, "\t%s: subpool name pool_id is %d\n", SELF,
                                qp->subpool_name_status[i].pool_id.pool);

                        CDR_MSG(4, "\t%s: query subpool name status is %s\n", SELF,
                                cl_status(sp->status));
                    }

                    CDR_MSG(4, "\t%s: subpool name count is %d\n", SELF, qp->count);
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_query_volume"
/*********************************************************************/
/* Input count of volsers to return, plus volser strings for         */
/* each volser desired.  A count of 0 returns all volsers.           */
/*                                                                   */
/* Format of input volser(s) is:                                     */
/* "VVVVVV" with each volser being 6 characters.                     */
/* Example: "TIM021,S00023"                                          */
/*********************************************************************/
void c_query_volume(int count, ...)
{
    int                 i;
    int                 index;
    CELLID              cell_id;
    VOLID               vol_id[MAX_ID];
    char               *pVolIdString;
    va_list             volIds;
    QU_VOL_STATUS      *sp;
    ACS_QUERY_VOL_RESPONSE *qp;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO)509;
    status = acs_query_volume(s, vol_id, count);

    if (STATUS_SUCCESS != status)
    {
        CDR_MSG(0, "\t%s: acs_query_volume() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_QUERY_VOL_RESPONSE *) rbuf;

            if (qp->query_vol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE query_vol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->query_vol_status));
            }
            else
            {
                for (i = 0; i < (int) qp->count; i++)
                {
                    sp = &(qp->vol_status[i]);

                    if (!check_mode)
                    {
                        st_show_vol_info(i, 
                                         &(sp->status), 
                                         &sp->vol_id);

                        st_show_media_type(i, 
                                           sp->media_type);

                        if (sp->location_type != LOCATION_CELL)
                        {
                            st_show_drv_info(i, 
                                             NULL, 
                                             &(sp->location.drive_id));
                        }
                        else
                        {
                            st_show_cellid(i, 
                                           NULL, 
                                           &(sp->location.cell_id));
                        }
                    }
                    else
                    {
                        st_chk_vol_info(i, 
                                        &(sp->status), 
                                        STATUS_VOLUME_IN_TRANSIT, 
                                        &sp->vol_id, 
                                        &(vol_id[i]));

                        st_chk_media_type(i, 
                                          sp->media_type, 
                                          i + 5);

                        if (sp->location_type != LOCATION_CELL)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE location failure\n", SELF);
                            CDR_MSG(0, "\t%s: got %s, expected %s\n", SELF, 
                                    (sp->location_type == LOCATION_DRIVE)
                                    ? "LOCATION_DRIVE" : "LOCATION_INVALID", "LOCATION_CELL");
                        }
                        else
                        {
                            cell_id.panel_id.lsm_id.acs = (ACS) i;
                            cell_id.panel_id.lsm_id.lsm = (LSM) i + 1;
                            cell_id.panel_id.panel = (PANEL) i + 2;
                            cell_id.row = (ROW) i + 3;
                            cell_id.col = (COL) i + 4;

                            st_chk_cellid(i, 
                                          &sp->location.cell_id, 
                                          &cell_id);
                        }
                    }
                }
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_register"
void c_register()
{
    int                 i;
    unsigned short      count;
    REGISTRATION_ID     registration_id;
    EVENT_CLASS_TYPE    eventClass[MAX_ID];
    ACS_REGISTER_RESPONSE *from_server;

    /* First unregister; in case this client is registered... */
    CDR_MSG(0, "\t%s: Unregistering event notification prior to registration...\n", SELF);
    c_unregister();

    /* Now do the register... */
    CDR_MSG(0, "\t%s: Registering for event notification...\n", SELF);

    s = (SEQ_NO)404;
    strcpy(registration_id.registration, "reg_test");
    count = MAX_EVENT_CLASS_TYPE; 
    eventClass[0] = EVENT_CLASS_RESOURCE;
    eventClass[1] = EVENT_CLASS_VOLUME;
    eventClass[2] = EVENT_CLASS_DRIVE_ACTIVITY;

    status = acs_register(s, registration_id, eventClass, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_register() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else if (type == RT_INTERMEDIATE)
        {
            from_server = (ACS_REGISTER_RESPONSE *) rbuf;

            st_show_int_resp_hdr();

            if (from_server->register_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: INTERMEDIATE RESPONSE register_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(from_server->register_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_register_info(from_server);
                }

                if (real_server)
                {
                    CDR_MSG(0, "\t%s: Checking registration for event notification...\n", SELF);
                    c_check_registration();

                    CDR_MSG(0, "\t%s: Unregistering event notification...\n", SELF);
                    c_unregister();

                    s = (SEQ_NO)404;    
                    type = RT_INTERMEDIATE; 
                    CDR_MSG(0, "\t%s: Receive final register event notification packet...\n", SELF);
                }
            }
        }

        if (type == RT_FINAL)
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            from_server = (ACS_REGISTER_RESPONSE *) rbuf;

            if (from_server->register_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE register_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(from_server->register_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_register_info(from_server);
                }
            }
        }
    } while (type != RT_FINAL); 
}


#undef SELF
#define SELF "c_set_access"
void c_set_access(BOOLEAN set)
{
    char                usr_id[EXTERNAL_USERID_SIZE];
    char               *usr_id_ptr;

    usr_id[EXTERNAL_USERID_SIZE - 1] = '\0';
    usr_id_ptr = &usr_id[0];

    if (!check_mode)
    {
        if ((usr_id_ptr = getenv("ACSAPI_USER_ID")) == NULL)
        {
            CDR_MSG(0, "\t%s: environment variable %s not set\n", SELF, "ACSAPI_USER_ID"); 

            exit(1);
        }

        if (strlen(usr_id_ptr) > (size_t) EXTERNAL_USERID_SIZE)
        {
            CDR_MSG(0, "\t%s: user_id string is too long\n", SELF);

            exit(1);
        }

        if (!set)
        {
            usr_id_ptr = NULL;
        }
    }
    else
    {
        strcpy(usr_id_ptr, "t_cdriver");
    }

    status = acs_set_access(usr_id_ptr);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_set_access() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    return;
}


#undef SELF
#define SELF "c_set_cap"
/*********************************************************************/
/* Input cap priority number, plus cap mode, plus number of caps,    */
/* plus capId strings for each cap to set.                           */
/* NOTE: A count of 0 should return an error.                        */
/*                                                                   */
/* Format of input capId(s) is:                                      */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*********************************************************************/
void c_set_cap(CAP_PRIORITY cap_priority,
               CAP_MODE     cap_mode,
               int          count, ...)
{
    CAPID               cap_id[MAX_ID];
    char               *pCapIdString;
    char                capIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             capIds;
    ACS_SET_CAP_RESPONSE *sp;

    va_start(capIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pCapIdString = va_arg(capIds, char*);
        memset(capIdSubstring, 0 , sizeof(capIdSubstring));
        memcpy(capIdSubstring, &(pCapIdString[0]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.acs = (ACS) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[4]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.lsm = (LSM) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[8]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].cap = (CAP) wkInt;
    }

    va_end(capIds);

    s = (SEQ_NO)012;
    status = acs_set_cap(s, cap_priority, cap_mode, cap_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_set_cap() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            sp = (ACS_SET_CAP_RESPONSE *) rbuf;

            if (sp->set_cap_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(sp->set_cap_status));
            }

            if (!check_mode)
            {
                st_show_cap_misc(0, 
                                 NULL, 
                                 sp->cap_mode, 
                                 sp->cap_priority);

                for (i = 0; i < sp->count; i++)
                {
                    st_show_cap_info(i, 
                                     &(sp->cap_status[i]), 
                                     &(sp->cap_id[i]));
                }
            }
            else
            {
                st_chk_cap_misc(0, 
                                NULL, 
                                STATE_ONLINE, 
                                sp->cap_mode, 
                                cap_mode, 
                                sp->cap_priority, 
                                cap_priority);

                for (i = 0; i < sp->count; i++)
                {
                    st_chk_cap_info(i, 
                                    i, 
                                    &(sp->cap_status[i]), 
                                    STATUS_SUCCESS, 
                                    &(sp->cap_id[i]), 
                                    &(cap_id[i]));
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_set_clean"
/*********************************************************************/
/* Input BOOLEAN clean flag, plus numeric maximum use count,         */
/* plus integer LOCKID (or 0 or NO_LOCK_ID), plus number of          */
/* volser ranges, plus volser range strings for each volser          */
/* range to set.                                                     */
/* NOTE: A count of 0 should return an error.                        */
/*                                                                   */
/* Format of input volser range(s) is:                               */
/* "VVVVVV-VVVVVV" with each volser being 6 characters and first and */
/* last volser being separated by a dash ("-").  Note that first and */
/* last volser may be the same.  Alternatively, if input volser      */
/* range is only 6 characters, then range is assumed to be that      */
/* single volser.                                                    */
/* Example: "TIM001-TIM099", "S00042-S00042"                         */
/*********************************************************************/
void c_set_clean(BOOLEAN cleanFlag,
                 short   maxUse, 
                 int     lockId, 
                 int     count, ...)
{
    int                 i;
    int                 index;
    LOCKID              lock_id             = (LOCKID) lockId;
    VOLRANGE            vol_range[MAX_ID];
    char               *pVolrangeString;
    va_list             volranges;
    ACS_SET_CLEAN_RESPONSE *sp;

    va_start(volranges, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolrangeString = va_arg(volranges, char*);

        if (strlen(pVolrangeString) == 6)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   pVolrangeString,
                   6);
        }
        else if (strlen(pVolrangeString) == 13)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   &(pVolrangeString[7]),
                   6);
        }
    }

    va_end(volranges);

    s = (SEQ_NO)012;
    status = acs_set_clean(s, lock_id, maxUse, vol_range, cleanFlag, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_set_clean() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            sp = (ACS_SET_CLEAN_RESPONSE *) rbuf;

            if (sp->set_clean_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(sp->set_clean_status));
            }
            else
            {
                if (!check_mode)
                {
                    CDR_MSG(4, "\t%s: max_use is %d\n", SELF, sp->max_use);

                    for (i = 0; i < sp->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(sp->vol_status[i]), 
                                         &(sp->vol_id[i]));
                    }
                }
                else
                {
                    if (sp->max_use != maxUse)
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE max_use failure\n", SELF);
                    }
                    if (sp->count != count)
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE count failure\n", SELF);
                        CDR_MSG(0, "\t%s: got %d, expected %d\n", SELF, sp->count, 1);
                    }
                    else
                    {
                        st_chk_vol_info(0, 
                                        &(sp->vol_status[0]), 
                                        STATUS_SUCCESS, 
                                        &(sp->vol_id[0]), 
                                        &(vol_range[0].startvol));

                        if (sp->max_use != maxUse)
                        {
                            CDR_MSG(0, "\t%s: FINAL RESPONSE max_use failure\n", SELF);
                        }
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}

#undef SELF
#define SELF "c_set_scratch"
/*********************************************************************/
/* Input BOOLEAN unscratch flag, plus integer LOCKID (or 0           */
/* or NO_LOCK_ID), plus scratch subpool ID (or scratch subpool       */
/* index), plus number of volser ranges, plus volser range           */
/* strings for each volser range to scratch or unscratch.            */
/* NOTE: A count of 0 should return an error.                        */
/*                                                                   */
/* Format of input volser range(s) is:                               */
/* "VVVVVV-VVVVVV" with each volser being 6 characters and first and */
/* last volser being separated by a dash ("-").  Note that first and */
/* last volser may be the same.  Alternatively, if input volser      */
/* range is only 6 characters, then range is assumed to be that      */
/* single volser.                                                    */
/* Example: "TIM001-TIM099", "S00042-S00042"                         */
/*********************************************************************/
void c_set_scratch(BOOLEAN unscratchFlag, 
                   int     lockId, 
                   int     poolId, 
                   int     count, ...)
{
    int                 i;
    int                 index;
    LOCKID              lock_id             = (LOCKID) lockId;
    POOL                pool                = (POOL) poolId;
    VOLRANGE            vol_range[MAX_ID];
    char               *pVolrangeString;
    va_list             volranges;
    ACS_SET_SCRATCH_RESPONSE *sp;

    va_start(volranges, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolrangeString = va_arg(volranges, char*);

        if (strlen(pVolrangeString) == 6)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   pVolrangeString,
                   6);
        }
        else if (strlen(pVolrangeString) == 13)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   &(pVolrangeString[7]),
                   6);
        }
    }

    va_end(volranges);

    s = (SEQ_NO)012;
    status = acs_set_scratch(s, lock_id, pool, vol_range, unscratchFlag, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_set_scratch() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            sp = (ACS_SET_SCRATCH_RESPONSE *) rbuf;

            if (sp->set_scratch_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(sp->set_scratch_status));
                CDR_MSG(0, "\t%s: set_scratch_status=%d (%08X), sp=08X\n", 
                        SELF, 
                        sp->set_scratch_status,
                        &(sp->set_scratch_status),
                        sp);
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < sp->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(sp->vol_status[i]), 
                                         &(sp->vol_id[i]));
                    }
                }
                else
                {
                    if (sp->pool != pool)
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE pool failure\n", SELF);
                    }
                    if (sp->count != 1)
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE count failure\n", SELF);
                    }

                    st_chk_vol_info(0, 
                                    &(sp->vol_status[0]), 
                                    STATUS_SUCCESS, 
                                    &(sp->vol_id[0]), 
                                    &(vol_range[0].startvol));
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_start"
void c_start()
{
    ACS_START_RESPONSE *sp;

    s = (SEQ_NO)012;
    status = acs_start(s);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_start() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            sp = (ACS_START_RESPONSE *) rbuf;

            if (sp->start_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(sp->start_status));
            }
        }
    } while (type != RT_FINAL);

    return;
}


#undef SELF
#define SELF "c_unlock_drive"
/*********************************************************************/
/* Input integer LOCKID, plus integer count of drive(s) to unlock,   */
/* plus driveId string(s).                                           */
/* NOTE: An input LOCKID of 0 or NO_LOCK_ID should return an error.  */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_unlock_drive(int lockId, 
                    int count, ...)
{
    LOCKID              lock_id             = (LOCKID) lockId;
    DRIVEID             drive_id[MAX_ID];
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    ACS_UNLOCK_DRV_RESPONSE *qp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);
    s = (SEQ_NO)509;
    status = acs_unlock_drive(s, lock_id, drive_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_query_lock_volume() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_UNLOCK_DRV_RESPONSE *) rbuf;

            if (qp->unlock_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE unlock_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->unlock_drv_status));
            }
            else
            {
                st_show_lo_drv_status(qp->count, 
                                      &(qp->drv_status[0]), 
                                      &(drive_id[0]),
                                      SELF);
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_unlock_volume"
/*********************************************************************/
/* Input integer LOCKID, plus integer count of volser(s) to unlock,  */
/* plus volser string(s).                                            */
/* NOTE: An input LOCKID of 0 or NO_LOCK_ID should return an error.  */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_unlock_volume(int lockId, 
                     int count, ...)
{
    VOLID               vol_id[MAX_ID];
    LOCKID              lock_id = (LOCKID) lockId;
    char               *pVolIdString;
    int                 i;
    va_list             volIds;
    ACS_UNLOCK_VOL_RESPONSE *qp;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    s = (SEQ_NO)509;
    status = acs_unlock_volume(s, lock_id, vol_id, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_unlock_volume() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            qp = (ACS_UNLOCK_VOL_RESPONSE *) rbuf;

            if (qp->unlock_vol_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE unlock_vol_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(qp->unlock_vol_status));
            }
            else
            {
                st_show_lo_vol_status(qp->count, 
                                      &(qp->vol_status[0]), 
                                      &(vol_id[0]),
                                      SELF);
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_unregister"
void c_unregister()
{
    ACS_UNREGISTER_RESPONSE *from_server;
    int             i;
    LOCKID      lock_id = NO_LOCK_ID;
    REGISTRATION_ID registration_id;
    EVENT_CLASS_TYPE eventClass[MAX_ID];
    unsigned short  count;

    s = (SEQ_NO)409;
    strcpy(registration_id.registration, "reg_test");
    count = 3; /* Must equal size of array passed */
    eventClass[0] = EVENT_CLASS_RESOURCE;
    eventClass[1] = EVENT_CLASS_VOLUME;
    eventClass[2] = EVENT_CLASS_DRIVE_ACTIVITY;
    status = acs_unregister(s, registration_id, eventClass, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_unregister() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            from_server = (ACS_UNREGISTER_RESPONSE *) rbuf;

            if (from_server->unregister_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE unregister_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", 
                        SELF, acs_status(from_server->unregister_status));
            }

            st_show_event_register_status(&from_server->event_register_status);
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_vary_acs"
/*********************************************************************/
/* Input state, plus BOOLEAN force flag, plus integer count of       */
/* acsIds to vary, plus acsId string for each acsId to set.          */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input acsId(s) is:                                      */
/* "decimal ACS" with ACS being 3 digit string.                      */
/* Example: "127" (ACS=127)                                          */
/*********************************************************************/
void c_vary_acs(STATE   state,
                BOOLEAN force,
                int     count, ...)
{
    ACS                 acs[MAX_ID];
    char               *pAcsIdString;
    char                acsIdString[4];
    int                 wkInt;
    int                 i;
    va_list             acsIds;
    ACS_VARY_ACS_RESPONSE *vp;

    va_start(acsIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pAcsIdString = va_arg(acsIds, char*);
        memset(acsIdString, 0 , sizeof(acsIdString));
        memcpy(acsIdString, &pAcsIdString[0], 3);
        wkInt = atoi(acsIdString);
        acs[i] = (ACS) wkInt;
    }

    va_end(acsIds);

    s = (SEQ_NO)601;
    status = acs_vary_acs(s, acs, state, force, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_vary_acs() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, (force? EXTENDED | ACKNOWLEDGE | FORCE : EXTENDED | ACKNOWLEDGE), 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            vp = (ACS_VARY_ACS_RESPONSE *) rbuf;

            if (vp->vary_acs_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE vary_acs_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(vp->vary_acs_status));
            }
            if (!check_mode)
            {
                st_show_state(0, 
                              vp->acs_state);

                for (i = 0; i < vp->count; i++)
                {
                    CDR_MSG(4, "\t%s: acs[%d] is %d.\n", SELF, i, vp->acs[i]);

                    CDR_MSG(4, "\t%s: acs[%d] status = %s\n", 
                            SELF, i, acs_status(vp->acs_status[i]));
                }
            }
            else
            {
                st_chk_state(0, 
                             vp->acs_state, 
                             state);

                for (i = 0; i < vp->count; i++)
                {
                    if (vp->acs[i] != acs[i])
                    {
                        CDR_MSG(0, "\t%s: FINAL RESPONSE acs[%d] failure\n", SELF, i);
                        CDR_MSG(0, "\t%s: got %d, expected %d\n", SELF, vp->acs[i], acs[i]);
                    }

                    if (vp->acs_status[i] != STATUS_SUCCESS)
                    {
                        CDR_MSG(0, "\t%s: %s acs[%d] status failure\n", SELF, fin_resp_str, i);
                        CDR_MSG(0, "\t%s: got %s, expected %s\n", 
                                SELF, acs_status(vp->acs_status[i]), acs_status(STATUS_SUCCESS));
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_vary_cap"
/*********************************************************************/
/* Input state, plus integer count of                                */
/* capIds to vary, plus capId string for each capId to set.          */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input capId(s) is:                                      */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*********************************************************************/
void c_vary_cap(STATE state,
                int   count, ...)
{
    CAPID               cap_id[MAX_ID];
    char               *pCapIdString;
    char                capIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             capIds;
    ACS_VARY_CAP_RESPONSE *vp;

    va_start(capIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pCapIdString = va_arg(capIds, char*);
        memset(capIdSubstring, 0 , sizeof(capIdSubstring));
        memcpy(capIdSubstring, &(pCapIdString[0]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.acs = (ACS) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[4]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].lsm_id.lsm = (LSM) wkInt;

        memcpy(capIdSubstring, &(pCapIdString[8]), 3);
        wkInt = atoi(capIdSubstring);
        cap_id[i].cap = (CAP) wkInt;
    }

    va_end(capIds);

    s = (SEQ_NO)605;
    status = acs_vary_cap(s, cap_id, state, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_vary_cap() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            vp = (ACS_VARY_CAP_RESPONSE *) rbuf;

            if (vp->vary_cap_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE vary_cap_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(vp->vary_cap_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_state(0, 
                                  vp->cap_state);

                    for (i = 0; i < vp->count; i++)
                    {
                        st_show_cap_info(i, 
                                         &vp->cap_status[i], 
                                         &vp->cap_id[i]);
                    }
                }
                else
                {
                    st_chk_state(0, 
                                 vp->cap_state, 
                                 state);

                    for (i = 0; i < vp->count; i++)
                    {
                        st_chk_cap_info(i, 
                                        i, 
                                        &vp->cap_status[i], 
                                        STATUS_SUCCESS, 
                                        &vp->cap_id[i], 
                                        &cap_id[i]);
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_vary_drive"
/*********************************************************************/
/* Input state, plus LOCKID (or 0 or NO_LOCK_ID), plus integer       */
/* count of driveIds to vary, plus driveId string for each driveId   */
/* to set.                                                           */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input driveId is:                                       */
/* decimal "ACS:decimal LSM:decimal panel:decimal drive"             */
/* with each element being 3 digits separated by ":".                */
/* Example: "127:000:001:002" (ACS=127, LSM=000, PANEL=001,          */
/* drive=002).                                                       */
/*********************************************************************/
void c_vary_drive(STATE state,
                  int   lockId, 
                  int   count, ...)
{
    LOCKID              lock_id             = (LOCKID) lockId;
    DRIVEID             drive_id[MAX_ID];
    char               *pDriveIdString;
    char                driveIdSubstring[4];
    int                 wkInt;
    int                 i;
    va_list             driveIds;
    ACS_VARY_DRV_RESPONSE *vp;

    va_start(driveIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pDriveIdString = va_arg(driveIds, char*);
        memcpy(driveIdSubstring, &(pDriveIdString[0]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.acs = (ACS) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[4]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.lsm_id.lsm = (LSM) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[8]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].panel_id.panel = (PANEL) wkInt;

        memcpy(driveIdSubstring, &(pDriveIdString[12]), 3);
        wkInt = atoi(driveIdSubstring);
        drive_id[i].drive = (DRIVE) wkInt;
    }

    va_end(driveIds);

    s = (SEQ_NO)602;
    status = acs_vary_drive(s, lock_id, drive_id, state, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_vary_drive() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, lock_id);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            vp = (ACS_VARY_DRV_RESPONSE *) rbuf;

            if (vp->vary_drv_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE vary_drv_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(vp->vary_drv_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < vp->count; i++)
                    {
                        st_show_drv_info(i, 
                                         &vp->drive_status[i], 
                                         &vp->drive_id[i]);
                    }

                    st_show_state(0, vp->drive_state);
                }
                else
                {
                    for (i = 0; i < vp->count; i++)
                    {
                        st_chk_drv_info(i, 
                                        &(vp->drive_status[i]), 
                                        STATUS_SUCCESS, 
                                        &(vp->drive_id[i]), 
                                        &(drive_id[i]));
                    }

                    st_chk_state(0, 
                                 vp->drive_state, 
                                 state);
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_vary_lsm"
/*********************************************************************/
/* Input state, plus BOOLEAN force flag, plus integer count of       */
/* lsmIds to vary, plus lsmId string for each lsmId to set.          */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input lsmId(s) is:                                      */
/* "decimal ACS:decimal LSM"                                         */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002" (ACS=001, LSM=002)                             */
/*********************************************************************/
void c_vary_lsm(STATE   state,
                BOOLEAN force, 
                int     count, ...)
{
    LSMID               lsm_id[MAX_ID];
    int                 i;
    char               *pLsmIdString;
    char                lsmIdSubstring[4];
    int                 wkInt;
    va_list             lsmIds;
    ACS_VARY_LSM_RESPONSE *vp;

    va_start(lsmIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pLsmIdString = va_arg(lsmIds, char*);
        memset(lsmIdSubstring, 0 , sizeof(lsmIdSubstring));
        memcpy(lsmIdSubstring, &(pLsmIdString[0]), 3);
        wkInt = atoi(lsmIdSubstring);
        lsm_id[i].acs = (ACS) wkInt;

        memcpy(lsmIdSubstring, &(pLsmIdString[4]), 3);
        wkInt = atoi(lsmIdSubstring);
        lsm_id[i].lsm = (LSM) wkInt;
    }

    va_end(lsmIds);

    s = (SEQ_NO)603;
    status = acs_vary_lsm(s, lsm_id, state, force, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_vary_lsm() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, (force? EXTENDED | ACKNOWLEDGE | FORCE : EXTENDED | ACKNOWLEDGE), 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            vp = (ACS_VARY_LSM_RESPONSE *) rbuf;

            if (vp->vary_lsm_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE vary_lsm_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(vp->vary_lsm_status));
            }
            else
            {
                if (!check_mode)
                {
                    for (i = 0; i < vp->count; i++)
                    {
                        st_show_lsm_id(i, 
                                       &vp->lsm_status[i], 
                                       &vp->lsm_id[i]);
                    }

                    st_show_state(0, 
                                  vp->lsm_state);
                }
                else
                {
                    for (i = 0; i < vp->count; i++)
                    {
                        st_chk_lsm_id(i, 
                                      &vp->lsm_status[i], 
                                      STATUS_SUCCESS, 
                                      &vp->lsm_id[i], 
                                      &lsm_id[i]);
                    }

                    st_chk_state(0, 
                                 vp->lsm_state, 
                                 state);
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_vary_port"
/*********************************************************************/
/* Input state, plus integer count of                                */
/* portIds to vary, plus portId string for each portId to set.       */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input portId(s) is:                                     */
/* "decimal ACS:decimal PORT"                                        */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002" (ACS=001, PORT=002)                            */
/*********************************************************************/
void c_vary_port(STATE state,
                 int   count, ...)
{
    PORTID              port_id[MAX_ID];
    int                 i;
    char               *pPortIdString;
    char                portIdSubstring[4];
    int                 wkInt;
    va_list             portIds;
    ACS_VARY_PRT_RESPONSE *vp;

    va_start(portIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pPortIdString = va_arg(portIds, char*);
        memset(portIdSubstring, 0 , sizeof(portIdSubstring));
        memcpy(portIdSubstring, &pPortIdString[0], 3);
        wkInt = atoi(portIdSubstring);
        port_id[i].acs = (ACS) wkInt;

        memcpy(portIdSubstring, &pPortIdString[4], 3);
        wkInt = atoi(portIdSubstring);
        port_id[i].port = (PORT) wkInt;
    }

    va_end(portIds);

    s = (SEQ_NO)604;
    status = acs_vary_port(s, port_id, state, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_vary_port() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, 0, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            vp = (ACS_VARY_PRT_RESPONSE *) rbuf;

            if (vp->vary_prt_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE vary_prt_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(vp->vary_prt_status));
            }
            else
            {
                for (i = 0; i < vp->count; i++)
                {
                    if (!check_mode)
                    {
                        st_show_port_info(i, 
                                          &vp->port_id[i], 
                                          vp->port_state, 
                                          vp->port_status[i]);
                    }
                    else
                    {
                        st_chk_port_info(i, 
                                         &vp->port_id[i], 
                                         &port_id[i], 
                                         vp->port_state, 
                                         state, 
                                         vp->port_status[i], 
                                         STATUS_SUCCESS);
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_venter"
/*********************************************************************/
/* Input capId string, plus integer count of unlabelled cartridges,  */
/* plus "virtual" volid string for each unlabelled cartridge that    */
/* will be associated with the unlabelled cartrdige.                 */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input capId is:                                         */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*                                                                   */
/* Format of input volid is 6 character volser string.               */
/* Example: "ACT001" (volser ACT001).                                */
/*********************************************************************/
void c_venter(char capId[11],
              int  count, ...)
{
    int                 i;
    int                 wkInt;
    char                capIdSubstring[4];
    CAPID               cap_id;
    VOLID               vol_id[MAX_ID];
    char               *pVolIdString;
    va_list             volIds;
    ACS_ENTER_RESPONSE *ep;

    va_start(volIds, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolIdString = va_arg(volIds, char*);
        strcpy(vol_id[i].external_label, pVolIdString);
    }

    va_end(volIds);

    memset(capIdSubstring, 0 , sizeof(capIdSubstring));
    memcpy(capIdSubstring, &(capId[0]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.acs = (ACS) wkInt;

    memcpy(capIdSubstring, &(capId[4]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(capIdSubstring, &(capId[8]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.cap = (CAP) wkInt;

    s = (SEQ_NO)402;

    /* If not a simulated server, then skip this request */
    if ((real_server) &
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_venter(s, cap_id, count, vol_id);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_venter() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, VIRTUAL, NO_LOCK_ID);
        }
        else
        {
            st_show_final_resp_hdr();

            if (no_variable_part() == TRUE)
            {
                break;
            }

            ep = (ACS_ENTER_RESPONSE *) rbuf;

            if (ep->enter_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE enter_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(ep->enter_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_cap_info(0, 
                                     NULL, 
                                     &(ep->cap_id));

                    for (i = 0; i < ep->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(ep->vol_status[i]), 
                                         &(ep->vol_id[i]));
                    }
                }
                else
                {
                    st_chk_cap_info(ep->count, 
                                    count, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(ep->cap_id), 
                                    &(cap_id));

                    for (i = 0; i < ep->count; i++)
                    {
                        st_chk_vol_info(i, 
                                        &(ep->vol_status[i]), 
                                        STATUS_SUCCESS, 
                                        &(ep->vol_id[i]), 
                                        &(vol_id[i]));
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


#undef SELF
#define SELF "c_xeject"
/*********************************************************************/
/* Input LOCKID (or 0 or NO_LOCK_ID), plus capId string,             */
/* plus integer count of volser ranges, plus volser range            */
/* string for each volser range to eject.                            */
/* NOTE: An input count of 0 should return an error.                 */
/*                                                                   */
/* Format of input capId is:                                         */
/* "decimal ACS:decimal LSM:decimal CAP"                             */
/* with each element being 3 digits separated by ":".                */
/* Example: "001:002:005" (ACS=001, LSM=002, CAP=005)                */
/*                                                                   */
/* Format of input volser range(s) is:                               */
/* "VVVVVV-VVVVVV" with each volser being 6 characters and first and */
/* last volser being separated by a dash ("-").  Note that first and */
/* last volser may be the same.  Alternatively, if input volser      */
/* range is only 6 characters, then range is assumed to be that      */
/* single volser.                                                    */
/* Example: "TIM001-TIM099", "S00042-S00042"                         */
/*********************************************************************/
void c_xeject(int  lockId, 
              char capId[11], 
              int  count, ...)
{
    int                 i;
    int                 wkInt;
    CAPID               cap_id;
    LOCKID              lock_id             = (LOCKID) lockId;
    VOLRANGE            vol_range[MAX_ID];
    char               *pVolrangeString;
    va_list             volranges;
    char                capIdSubstring[4];
    ACS_EJECT_RESPONSE *ep;

    va_start(volranges, count);

    for (i = 0; 
        i < count;
        i++)
    {
        pVolrangeString = va_arg(volranges, char*);

        if (strlen(pVolrangeString) == 6)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   pVolrangeString,
                   6);
        }
        else if (strlen(pVolrangeString) == 13)
        {
            memcpy(vol_range[i].startvol.external_label, 
                   pVolrangeString,
                   6);

            memcpy(vol_range[i].endvol.external_label, 
                   &(pVolrangeString[7]),
                   6);
        }
    }

    va_end(volranges);

    memset(capIdSubstring, 0 , sizeof(capIdSubstring));
    memcpy(capIdSubstring, &(capId[0]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.acs = (ACS) wkInt;

    memcpy(capIdSubstring, &(capId[4]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.lsm_id.lsm = (LSM) wkInt;

    memcpy(capIdSubstring, &(capId[8]), 3);
    wkInt = atoi(capIdSubstring);
    cap_id.cap = (CAP) wkInt;

    s = (SEQ_NO)403;

    /* If not a simulated server, then skip this request */
    if ((real_server) &
        (!(xapi)))
    {
        CDR_MSG(0, "\tThis command not checked against a real server... returning\n");

        return;
    }

    status = acs_xeject(s, lock_id, cap_id, vol_range, count);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_xeject() failed %s\n", SELF, acs_status(status));

        exit((int) status);
    }

    /* Loop to get all responses until FINAL response */
    do
    {
        status = acs_response(-1, &seq_nmbr, &req_id, &type, (char*) rbuf);

        if (status == STATUS_IPC_FAILURE)
        {
            exit((int) status);
        }

        if (type == RT_ACKNOWLEDGE)
        {
            st_show_ack_res(s, EXTENDED | ACKNOWLEDGE, RANGE, lock_id);
        }
        else
        {
            if (type == RT_INTERMEDIATE)
            {
                st_show_int_resp_hdr();
            }
            else
            {
                st_show_final_resp_hdr();
            }

            if (no_variable_part() == TRUE)
            {
                break;
            }

            ep = (ACS_EJECT_RESPONSE *) rbuf;

            if (ep->eject_status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE eject_status failure\n", SELF);
                CDR_MSG(0, "\t%s: status is %s\n", SELF, acs_status(ep->eject_status));
            }
            else
            {
                if (!check_mode)
                {
                    st_show_cap_requested(&(ep->cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        st_show_vol_info(i, 
                                         &(ep->vol_status[i]), 
                                         &(ep->vol_id[i]));

                        st_show_cap_info(-1, 
                                         NULL, 
                                         &(ep->cap_used[i]));
                    }
                }
                else
                {
                    st_chk_cap_info(ep->count, 
                                    count, 
                                    NULL, 
                                    STATUS_SUCCESS, 
                                    &(ep->cap_id), 
                                    &(cap_id));

                    for (i = 0; i < (int) ep->count; i++)
                    {
                        st_chk_vol_info(i, 
                                        &(ep->vol_status[i]), 
                                        STATUS_SUCCESS, 
                                        &(ep->vol_id[i]), 
                                        &(vol_range[i].startvol));

                        st_chk_cap_info(ep->count, 
                                        count, 
                                        NULL, 
                                        STATUS_SUCCESS, 
                                        &(ep->cap_used[i]), 
                                        &(cap_id));
                    }
                }
            }
        }
    }
    while (type != RT_FINAL);
}


/*********************************************************************/
/* The print functions start here.                                   */
/*********************************************************************/

/*
 * Name:
 *
 *        st_show_ack_res()
 *
 * Description:
 *
 *        This function prints acknowledge response data from the server
 *        for every request made by main. It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Implicit Inputs:
 *        
 *        rbuf - the buffer that holds response data returned by acs_response.
 *
 * Return Values:
 *
 *        None
 *
 */
#undef SELF
#define SELF "st_show_ack_res"
void st_show_ack_res(SEQ_NO        sn, 
                     unsigned char mopts, 
                     unsigned char eopts, 
                     LOCKID        lid)
{
    MESSAGE_HEADER  *msg_hdr_ptr;

    ap = (ACKNOWLEDGE_RESPONSE *) rbuf;
    msg_hdr_ptr = &(ap->request_header.message_header);
    CDR_MSG(0, "\t%s: ACKNOWLEDGE RESPONSE received\n", SELF);

    if (!check_mode)
    {
        switch (type)
        {
        case RT_ACKNOWLEDGE:
            CDR_MSG(4, "\t%s: Status = %s\n", SELF, acs_status(status));
            CDR_MSG(4, "\t%s: Sequence Number = %d\n", SELF, seq_nmbr);
            CDR_MSG(4, "\t%s: Msg Opts = %s (0x%x)\n", 
                    SELF, 
                    decode_mopts(msg_hdr_ptr->message_options), 
                    msg_hdr_ptr->message_options);
            CDR_MSG(4, "\t%s: Version = %s (%d)\n",
                    SELF, 
                    decode_vers((long) msg_hdr_ptr->version), 
                    msg_hdr_ptr->version);
            CDR_MSG(4, "\t%s: extended_options %s = 0x%x\n", 
                    SELF, 
                    decode_eopts((unsigned char) msg_hdr_ptr->extended_options), 
                    msg_hdr_ptr->extended_options);
            CDR_MSG(4, "\t%s: lock_id = 0x%x\n", SELF, msg_hdr_ptr->lock_id);
            CDR_MSG(4, "\t%s: status = %s\n", SELF, acs_status(ap->message_status.status));
            CDR_MSG(4, "\t%s: type = %s\n", SELF, acs_type(ap->message_status.type));
            CDR_MSG(4, "\t%s: message_id = %d \n", SELF, ap->message_id); 
            break;

        default:
            CDR_MSG(0, "\n\t%s: Got %s RESPONSE when expecting ACK RESPONSE\n", 
                    SELF, 
                    st_rttostr(type));
            break;
        }
    }
    else
    {
        if (msg_hdr_ptr->message_options != mopts)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE message_options failure\n", SELF);
        }

        if (msg_hdr_ptr->version != VERSION_LAST - 1)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE version failure\n", SELF);
        }

        if (msg_hdr_ptr->extended_options != eopts)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE extended_options failure\n", SELF);
        }

        if (msg_hdr_ptr->lock_id != lid)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE lock_id failure\n", SELF);
        }

        if (ap->message_status.status != STATUS_VALID)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE status failure\n", SELF);
        }

        if (ap->message_status.type != TYPE_NONE)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE type failure\n", SELF);
        }

        if (ap->message_id != 1)
        {
            CDR_MSG(0, "\t%s: ACK RESPONSE message_id failure\n", SELF);
        }
    }
}


/*
 * Name:
 *
 *        st_show_audit_int_res()
 *
 * Description:
 *
 *        This function prints intermediate response data from the server
 *        for every AUDIT request made by main. It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Implicit Inputs:
 *        
 *        rbuf - the buffer that holds response data returned by acs_response.
 *        status - Global variable that holds the status returned from 
 *                 acs_response.
 *        type - Global variable that holds the value of the response 
 *               type returned by acs_response.
 *        seq_number - Global variable that holds the user supplied sequence
 *                     number that uniquely identifies the response and which
 *                     is returned by acs_response.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_audit_int_res"
void st_show_audit_int_res(char *callingRoutine)
{
    unsigned int    cntr;

    CDR_MSG(0, "\t%s: INTERMEDIATE response received\n", SELF);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_response() (INTERMEDIATE) failed:%s\n", SELF, acs_status(status));

        return;
    }

    if (seq_nmbr != s)
    {
        CDR_MSG(0, "\t%s: INT sequence mismatch got:%d expected:%d\n", SELF, seq_nmbr, s);

        return;
    }

    if (type != RT_INTERMEDIATE)
    {
        CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE type failure\n", SELF);

        return;
    }

    aip = (ACS_AUDIT_INT_RESPONSE *) rbuf;

    if (!check_mode)
    {
        CDR_MSG(4, "\t  Intermediate Status: %s\n", acs_status(aip->audit_int_status));
        CDR_MSG(4, "\t%s:  cap's acs = %d\n", SELF, aip->cap_id.lsm_id.acs);
        CDR_MSG(4, "\t%s:  cap's lsm = %d\n", SELF, aip->cap_id.lsm_id.lsm);
        CDR_MSG(4, "\t%s:  cap's id = %d\n", SELF, aip->cap_id.cap);
        CDR_MSG(4, "\t%s:  count = %d\n", SELF, aip->count);

        for (cntr = 0; cntr < aip->count; ++cntr)
        {
            CDR_MSG(4, "\t%s:   vol_id[%d] = %s\n", 
                    SELF, cntr, aip->vol_id[cntr].external_label);
            CDR_MSG(4, "\t%s:   vol_status[%d] =  %s\n", 
                    SELF, cntr, acs_status(aip->vol_status[cntr]));
        }
    }
    else
    {
        if (aip->audit_int_status != STATUS_ACS_NOT_IN_LIBRARY)
        {
            CDR_MSG(0, "\n\t%s:  INTERMEDIATE RESPONSE audit_int_status failure\n", SELF);
        }

        if (aip->cap_id.lsm_id.acs != 0)
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE cap_acs failure\n", SELF);
        }

        if (aip->cap_id.lsm_id.lsm != 0)
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE cap_lsm failure\n", SELF);
        }

        if (aip->cap_id.cap != 0)
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE cap failure\n", SELF);
        }

        if ((aip->count != 1 && strcmp(callingRoutine, "c_audit_server") != 0) || 
            (aip->count != 0 && strcmp(callingRoutine, "c_audit_server") == 0))
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE count failure\n", SELF);
        }

        if ((strcmp(callingRoutine, "c_audit_server") != 0) && 
            (strcmp(aip->vol_id[0].external_label, "SPE007") != 0))
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE vol_id failure\n", SELF);
        }

        if ((strcmp(callingRoutine, "c_audit_server") != 0) && 
            (aip->vol_status[0] != STATUS_VOLUME_NOT_IN_LIBRARY))
        {
            CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE vol_status failure\n", SELF);
        }
    }
}


/*
 * Name:
 *
 *        st_show_int_resp_hdr()
 *
 * Description:
 *
 *        This function prints the intermediate response header
 *        data returned from acs_response().
 *        It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Implicit Inputs:
 *
 *        status - Global variable that holds the status returned from
 *                 acs_response.
 *        type - Global variable that holds the value of the response
 *               type returned by acs_response.
 *        seq_number - Global variable that holds the user supplied sequence
 *                     number that uniquely identifies the response and which
 *                     is returned by acs_response.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_int_resp_hdr"
void st_show_int_resp_hdr()
{
    CDR_MSG(0, "\t%s: INTERMEDIATE RESPONSE received\n", SELF);

    if (status != STATUS_SUCCESS)
    {
        CDR_MSG(0, "\t%s: acs_response() (int RESPONSE) failed:%s\n", 
                SELF, acs_status(status));

        return;
    }

    if (seq_nmbr != s)
    {
        CDR_MSG(0, "\t%s: INTERMEDIATE RESPONSE sequence mismatch got:%d expected:%d\n", 
                SELF, seq_nmbr, s);

        return;
    }

    if (!check_mode)
    {
        CDR_MSG(4, "\tIntermediate Status: %s\n", acs_status(status));
        CDR_MSG(4, "\tSequence Number: %d\n", seq_nmbr);
        CDR_MSG(4, "\tResponse Type: %s\n", st_rttostr(type));
    }
}


/*
 * Name:
 *
 *        st_show_final_resp_hdr()
 *
 * Description:
 *
 *        This function prints the response data returned from 
 *        acs_response(). It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Implicit Inputs:
 *        
 *        status - Global variable that holds the status returned from 
 *                 acs_response.
 *        type - Global variable that holds the value of the response 
 *               type returned by acs_response.
 *        seq_number - Global variable that holds the user supplied sequence
 *                     number that uniquely identifies the response and which
 *                     is returned by acs_response.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_final_resp_hdr"
void st_show_final_resp_hdr()
{
    CDR_MSG(0, "\t%s: FINAL RESPONSE received\n", SELF); 

    if ((status != STATUS_SUCCESS) && 
        (status != STATUS_DONE) && 
        (status != STATUS_RECOVERY_COMPLETE) && 
        (status != STATUS_NORMAL) && 
        (status != STATUS_INVALID_CAP) && 
        (status != STATUS_MULTI_ACS_AUDIT) && 
        (status != STATUS_VALID))
    {
        CDR_MSG(0, "\t%s: acs_response() (FINAL RESPONSE) failed:%s\n", 
                SELF, acs_status(status));

        return;
    }

    if (seq_nmbr != s)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE sequence mismatch got:%d expected:%d\n", 
                SELF, seq_nmbr, s);

        return;
    }

    if (type != RT_FINAL)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE type failure\n", SELF);
        CDR_MSG(0, "\t%s: expected RT_FINAL, got %s\n", SELF, acs_type_response(type));

        return;
    }

    if (!check_mode)
    {
        CDR_MSG(4, "\tFinal Status: %s\n", acs_status(status));
        CDR_MSG(4, "\tSequence Number: %d\n", seq_nmbr);
        CDR_MSG(4, "\tResponse Type: %s\n", st_rttostr(type));
    }
}


/*
 * Name:
 *
 *        st_show_state()
 *
 * Description:
 *
 *        This function prints state data from the server for several QUERY 
 *        and AUDIT requests made by main. It is used to print state info
 *        for lsms, acses, and drives. It only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        state - the state of the device.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_state"
void st_show_state(unsigned short i, 
                   STATE          state)
{
    CDR_MSG(4, "\t%s: state[%d] is %s\n", SELF, i, acs_state(state)); 
}


/*
 * Name:
 *
 *        st_chk_state()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        several QUERY and AUDIT requests made by main. It is used to 
 *        print state info for lsms, acses, and drives. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        state_got - the state of the device returned from the server.
 *        state_expected - the state of the device expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_state"
void st_chk_state(unsigned short i, 
                  STATE          state_got, 
                  STATE          state_expected)
{
    if (state_got != state_expected)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE state[%d] failure\n", SELF, i);
        CDR_MSG(0, "\t%s: got %s, expected %s\n", 
                SELF, 
                acs_state(state_got), 
                acs_state(state_expected));
    }
}


/*
 * Name:
 *
 *        st_show_cellid()
 *
 * Description:
 *
 *        This function prints cell data from the server for several QUERY 
 *        CLEAN, QUERY VOLUME, and QUERY SCRATCH requests made by main. 
 *        It is used to print cell info for tape volumes. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth cell being reported.
 *        stat - the status of the cell.
 *        from_server - the cellid of the nth cell being reported
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_cellid"
void st_show_cellid(unsigned short  i, 
                    STATUS         *stat, 
                    CELLID         *from_server)
{
    if (stat)
    {
        CDR_MSG(4, "\t%s: cellid[%d]: status is %s\n", 
                SELF, i, acs_status(*stat));
    }

    CDR_MSG(4, "\t%s: home location[%d]:  row is %d\n", SELF, i, from_server->row);
    CDR_MSG(4, "\t%s: home location[%d]:  column is %d\n", SELF, i, from_server->col);

    st_show_panel_info(i, NULL, &(from_server->panel_id));
}


/*
 * Name:
 *
 *        st_chk_cellid()
 *
 * Description:
 *
 *        This function prints cell data from the simulated server for 
 *        several QUERY CLEAN, QUERY VOLUME, and QUERY SCRATCH requests 
 *        made by main. It is used to print state info for lsms, 
 *        acses, and drives. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        got - pointer to the cell info structure returned.
 *        exp - pointer to the cell info expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_cellid"
void st_chk_cellid(unsigned short  i, 
                   CELLID         *got, 
                   CELLID         *exp)
{
    st_chk_lsm_id(i, 
                  NULL, 
                  STATUS_SUCCESS, 
                  &(got->panel_id.lsm_id), 
                  &(exp->panel_id.lsm_id));

    if (got->panel_id.panel != 
        exp->panel_id.panel)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE cellid [%d] panel failure\n", SELF, i);
    }

    if (got->row != exp->row)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE cellid [%d] row failure\n", SELF, i);
    }

    if (got->col != exp->col)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE cellid [%d] col failure\n", SELF, i);
    }
}


/*
 * Name:
 *
 *        st_show_drive_type()
 *
 * Description:
 *
 *        This function prints the drive type for the nth drive.
 *        It is called by st_show_drive_type_info and st_show_drive_status.
 *        It is not called by main. It only reports the values, it does no checking..
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive being reported.
 *        dtype - an integer that defines thetype of drive.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_drive_type"
void st_show_drive_type(unsigned short i, 
                        DRIVE_TYPE     dtype)
{
    char        dstr[80];

    switch (dtype)
    {
    case ANY_DRIVE_TYPE:
        sprintf(dstr, "ANY_DRIVE_TYPE");
        break;

    case UNKNOWN_DRIVE_TYPE:
        sprintf(dstr, "UNKNOWN_DRIVE_TYPE");
        break;

    case DRIVE_TYPE_4480:
        sprintf(dstr, "DRIVE_TYPE_4480");
        break;

    default:
        sprintf(dstr, "DRIVE_TYPE_%d", dtype);
    }

    CDR_MSG(4, "\t%s: drive type[%d] is %s\n", SELF, i, dstr);
}


/*
 * Name:
 *
 *        st_chk_drive_type()
 *
 * Description:
 *
 *        This function prints drive data from the simulated server for 
 *        several QUERY and AUDIT requests made by main. It is called by 
 *        st_chk_drive_type_info() and st_chk_drv_status().
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive being reported.
 *        got - the type of the drive returned from the server.
 *        expected - the type of the drive expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_drive_type"
void st_chk_drive_type(unsigned short i, 
                       DRIVE_TYPE     got, 
                       DRIVE_TYPE     expected)
{
    char        estr[80];
    char        gstr[80];

    if ((got != expected) && (acs_get_packet_version() > 3))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive type [%d] failure\n", SELF, i);

        switch (got)
        {
        case ANY_DRIVE_TYPE:
            sprintf(gstr, "ANY_DRIVE_TYPE");
            break;

        case UNKNOWN_DRIVE_TYPE:
            sprintf(gstr, "UNKNOWN_DRIVE_TYPE");
            break;

        case DRIVE_TYPE_4480:
            sprintf(gstr, "DRIVE_TYPE_4480");
            break;

        default:
            sprintf(gstr, "DRIVE_TYPE_%d", got);
        }

        switch (expected)
        {
        case ANY_DRIVE_TYPE:
            sprintf(estr, "ANY_DRIVE_TYPE");
            break;

        case UNKNOWN_DRIVE_TYPE:
            sprintf(estr, "UNKNOWN_DRIVE_TYPE");
            break;

        case DRIVE_TYPE_4480:
            sprintf(estr, "DRIVE_TYPE_4480");
            break;

        default:
            sprintf(estr, "DRIVE_TYPE_%d", expected);
        }

        CDR_MSG(0, "\t%s: got %s, expected %s\n", SELF, gstr, estr);
    }
}


/*
 * Name:
 *
 *        st_show_media_info()
 *
 * Description:
 *
 *        This function prints media data from the server for QUERY MIXED
 *        MEDIA requests made by main. The media information is for given 
 *        type of tape cartridge. It only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth media type being reported.
 *        mip - a pointer to the media type information.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_media_info"
void st_show_media_info(unsigned short        i, 
                        QU_MEDIA_TYPE_STATUS *mip)
{
    unsigned short  j;

    st_show_media_type(i, mip->media_type);

    CDR_MSG(4, "\t%s: media_info name[%d]: %s\n", SELF, i, mip->media_type_name);
    CDR_MSG(4, "\t%s:     clean capability   :%d\n", SELF, mip->cleaning_cartridge);
    CDR_MSG(4, "\t%s:     max cleaning usage :%d\n", SELF, mip->max_cleaning_usage);
    CDR_MSG(4, "\t%s:     compat_drive count :%d\n", SELF, mip->compat_count);

    for (j = 0; j < mip->compat_count; j++)
    {
        CDR_MSG(4, "\t%s:     compatible drive[%d] type : %d\n", 
                SELF, j, mip->compat_drive_types[j]);
    }
}


/*
 * Name:
 *
 *        st_chk_media_info()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        QUERY MIXED MEDIA requests made by main. The media information
 *        is for a given type of tape cartridge.
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth media type being reported.
 *        mip - pointer to the media type information returned from the server.
 *        mp - pointer to the media type information expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_media_info"
void st_chk_media_info(unsigned short        i, 
                       QU_MEDIA_TYPE_STATUS *mip, 
                       QU_MEDIA_TYPE_STATUS *mp)
{
    st_chk_media_type(i, 
                      mip->media_type, 
                      mp->media_type);

    if ((strcmp(mip->media_type_name, mp->media_type_name) != 0) && 
        (acs_get_packet_version() > 3))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE media_info name failure\n", SELF);
        CDR_MSG(0, "\t%s: got %s, expected %s\n", 
                SELF, mip->media_type_name, mp->media_type_name);
    }

    if (mip->cleaning_cartridge != mp->cleaning_cartridge)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE clean capability failure\n", SELF);
        CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                SELF, mip->cleaning_cartridge, mp->cleaning_cartridge);
    }

    if (mip->max_cleaning_usage != mp->max_cleaning_usage)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE max cleaning usage failure\n", SELF);
        CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                SELF, mip->max_cleaning_usage, mp->max_cleaning_usage);
    }

    if (mip->compat_count != mp->compat_count)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE compat_drive count failure\n", SELF);
        CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                SELF, mip->compat_count, mp->compat_count);
    }
    else
    {
        for (i = 0; i < mp->compat_count; i++)
        {
            if (mip->compat_drive_types[i] != 
                mp->compat_drive_types[i])
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE compat drive failure\n", SELF);
                CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                        SELF, mip->compat_drive_types[i], mp->compat_drive_types[i]);
            }
        }
    }
}


/*
 * Name:
 *
 *        st_show_drive_type_info()
 *
 * Description:
 *
 *        This function prints drive data from the server for QUERY MIXED
 *        MEDIA requests made by main. It only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive being reported.
 *        dip - pointer to the drive info structure returned by acs_response. 
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_drive_type_info"
void st_show_drive_type_info(unsigned short        i, 
                             QU_DRIVE_TYPE_STATUS *dip)
{
    unsigned short  j;

    st_show_drive_type(i, dip->drive_type);

    CDR_MSG(4, "\t%s: drive type name[%d] - %s\n", 
            SELF, i, dip->drive_type_name);

    for (j = 0; j < dip->compat_count; j++)
    {
        CDR_MSG(4, "\t%s:     compatible media[%d] - %d\n", 
                SELF, j, dip->compat_media_types[j]);
    }
}


/*
 * Name:
 *
 *        st_chk_drive_type_info()
 *
 * Description:
 *
 *        This function prints compatible drive data from the simulated 
 *        server for QUERY MIXED MEDIA requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive type being reported.
 *        dip - pointer to the drive type info returned from the server.
 *        dp - pointer to the drive type info expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_drive_type_info"
void st_chk_drive_type_info(unsigned short        i, 
                            QU_DRIVE_TYPE_STATUS *dip, 
                            QU_DRIVE_TYPE_STATUS *dp)
{
    st_chk_drive_type(i, 
                      dip->drive_type, 
                      dp->drive_type);

    if (strcmp(dip->drive_type_name, dp->drive_type_name) != 0)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive_info name failure\n", SELF);
        CDR_MSG(0, "\t%s: got %s , expected %s\n", 
                SELF, dip->drive_type_name, dp->drive_type_name);
    }

    if (dip->compat_count != dp->compat_count)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE compat_media_count failure\n", SELF);
        CDR_MSG(0, "\t%s: got %d , expected %d\n", 
                SELF, dip->compat_count, dp->compat_count);
    }
    else
    {
        for (i = 0; i < dp->compat_count; i++)
        {
            if (dip->compat_media_types[i] != 
                dp->compat_media_types[i])
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE compat media failure\n", SELF);
                CDR_MSG(0, "\t%s: got %d, expected %d\n", 
                        SELF, dip->compat_media_types[i], dp->compat_media_types[i]);
            }
        }
    }
}


/*
 * Name:
 *
 *        st_show_media_type()
 *
 * Description:
 *
 *        This function prints media type data from the server for QUERY 
 *        CLEAN, QUERY VOLUME, and QUERY SCRATCH requests made by main. 
 *        It is also called by st_show_media_info. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        mtype - integer that specifies the type of the tape cartridge.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_media_type"
void st_show_media_type(unsigned short i, 
                        MEDIA_TYPE     mtype)
{
    char        mstr[80];

    switch (mtype)
    {
    case ANY_MEDIA_TYPE:
        sprintf(mstr, "ANY_MEDIA_TYPE");
        break;

    case UNKNOWN_MEDIA_TYPE:
        sprintf(mstr, "UNKNOWN_MEDIA_TYPE");
        break;

    case MEDIA_TYPE_3480:
        sprintf(mstr, "MEDIA_TYPE_3480");
        break;

    default:
        sprintf(mstr, "MEDIA_TYPE_[%d]", mtype);
    }

    CDR_MSG(4, "\t%s: media type[%d] is %s\n", SELF, i, mstr);
}


/*
 * Name:
 *
 *        st_chk_media_type()
 *
 * Description:
 *
 *        This function prints media type data from the simulated server 
 *        for QUERY CLEAN, QUERY SCRATCH, and QUERY VOLUME requests 
 *        made by main. It is also called by st_chk_media_info().
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth media type being reported.
 *        got - the media type returned from the server.
 *        expected - the media type expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_media_type"
void st_chk_media_type(unsigned short i, 
                       MEDIA_TYPE     got, 
                       MEDIA_TYPE     expected)
{
    char        estr[80];
    char        gstr[80];

    if (acs_get_packet_version() > 3)
    {
        if (got != expected)
        {
            CDR_MSG(0, "\t%s: FINAL RESPONSE media type [%d] failure\n", SELF, i);

            switch (expected)
            {
            case ANY_MEDIA_TYPE:
                sprintf(estr, "ANY_MEDIA_TYPE");
                break;

            case UNKNOWN_MEDIA_TYPE:
                sprintf(estr, "UNKNOWN_MEDIA_TYPE");
                break;

            case MEDIA_TYPE_3480:
                sprintf(estr, "MEDIA_TYPE_3480");
                break;

            default:
                sprintf(estr, "MEDIA_TYPE_ %d", expected);
            }

            switch (got)
            {
            case ANY_MEDIA_TYPE:
                sprintf(gstr, "ANY_MEDIA_TYPE");
                break;

            case UNKNOWN_MEDIA_TYPE:
                sprintf(gstr, "UNKNOWN_MEDIA_TYPE");
                break;

            case MEDIA_TYPE_3480:
                sprintf(gstr, "MEDIA_TYPE_3480");
                break;

            default:
                sprintf(gstr, "MEDIA_TYPE_ %d", got);
            }

            CDR_MSG(0, "\t          got %s, expected %s\n", gstr, estr);
        }
    }
}


/*
 * Name:
 *
 *        st_show_usage()
 *
 * Description:
 *
 *        This function prints cleaning cartridge usage data from the server 
 *        for QUERY CLEAN requests made by main. It only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        cstat - pointer to a cleaning cartridge information structure. 
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_useage"
void st_show_usage(QU_CLN_STATUS *cstat)
{
    CDR_MSG(4, "\t        max use = %d, current use = %d\n", 
            cstat->max_use, cstat->current_use);
}


/*
 * Name:
 *
 *        st_chk_usage()
 *
 * Description:
 *
 *        This function prints cleaning cartridge usage  data from the 
 *        simulated server for QUERY CLEAN requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        got - ptr to the cleaning cartridge info returned from the server.
 *        expected - ptr to the cleaning cartridge info expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_useage"
void st_chk_usage(unsigned short  i, 
                  QU_CLN_STATUS  *got, 
                  QU_CLN_STATUS  *expected)
{
    if (got->max_use != expected->max_use)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE clean[%d] max_used failure\n", SELF, i);
        CDR_MSG(0, "\t           got %d, expected %d\n", 
                got->max_use, expected->max_use);
    }

    if (got->current_use != expected->current_use)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE clean[%d] current_use failure\n", SELF, i);
        CDR_MSG(0, "\t           got %d, expected %d\n", 
                got->current_use, expected->current_use);
    }
}


/*
 * Name:
 *
 *        st_show_acs_status()
 *
 * Description:
 *
 *        This function prints acs information from the server for QUERY 
 *        ACS requests made by main. It reports the acs' id, its state,
 *        its free cells, its status, and the status of its request.
 *        It only reports the values, it does no checking..
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        sp - pointer to the acs information structure. 
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_acs_status"
void st_show_acs_status(unsigned int   i, 
                        QU_ACS_STATUS *sp)
{
    CDR_MSG(4, "\t%s: acs[%d] acs_id = %d\n", SELF, i, sp->acs_id);

    st_show_state(i, sp->state);
    st_show_freecells(i, sp->freecells);

    CDR_MSG(4, "\t%s: acs[%d] status = %s\n", SELF, i, acs_status(sp->status));

    st_show_requests(i, &(sp->requests));
}


/*
 * Name:
 *
 *        st_chk_freecells()
 *
 * Description:
 *
 *        This function prints the number of free cells returned by 
 *        the simulated server for QUERY SERVER and QUERY LSM requests. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        got - the number of free cells returned from the server.
 *        expected - the number of free cells expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_freecells"
void st_chk_freecells(unsigned short i, 
                      FREECELLS      got, 
                      FREECELLS      expected)
{
    if (got != expected)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE freecells[%d] failure\n", SELF, i);
        CDR_MSG(0, "\t%s: got %d, expected %d\n", SELF, got, expected);
    }
}


/*
 * Name:
 *
 *        st_show_freecells()
 *
 * Description:
 *
 *        This function prints the number of free cells returned by a 
 *        server response to QUERY LSM and QUERY SERVER requests 
 *        made by main. It is also called by st_show_acs_status(). 
 *        It only reports the values, it does no checking..
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth server, acs, or lsm being reported.
 *        fcells - the count of unused cells in the server, acs, or lsm.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_freecells"
void st_show_freecells(unsigned short i, 
                       FREECELLS      fcells)
{
    CDR_MSG(4, "\t%s: freecells[%d] = %d\n", SELF, i, fcells); 
}


/*
 * Name:
 *
 *        st_show_requests()
 *
 * Description:
 *
 *        This function prints the request status returned by a 
 *        server response to QUERY LSM and QUERY SERVER requests 
 *        made by main. It is also called by st_show_acs_status(). 
 *        It only reports the values, it does no checking..
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        reqs - pointer to the array of pending and current requests.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_requests"
void st_show_requests(unsigned short  i, 
                      REQ_SUMMARY    *reqs)
{
    unsigned short  k;

    for (k = 0; k < (unsigned short) MAX_COMMANDS; k++)
    {
        CDR_MSG(4, "\t%s[%d]: Request %s - Current: %d, Pending: %d\n", 
                SELF, 
                i, 
                st_reqtostr(k), 
                reqs->requests[k][(int) CURRENT], 
                reqs->requests[k][(int) PENDING]); 
    }
}


/*
 * Name:
 *
 *        st_chk_requests()
 *
 * Description:
 *
 *        This function prints request status returned from the 
 *        simulated server for QUERY LSM and QUERY SERVER requests. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        got - pointer to array of pending and current requests returned.
 *        expected - pointer to array of pending/current requests expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_requests"
void st_chk_requests(REQ_SUMMARY *got, 
                     REQ_SUMMARY *expected)
{
    unsigned short  k;

    for (k = 0; k < 5; k++)
    {
        if ((got->requests[k][(int) CURRENT]) != 
            (expected->requests[k][(int) CURRENT]))
        {
            CDR_MSG(0, "\t%s: FINAL RESPONSE current request[%d] failure\n", SELF, k);
            CDR_MSG(0, "\t%s: got  %d, expected %d\n", 
                    SELF, 
                    got->requests[k][(int) CURRENT], 
                    expected->requests[k][(int) CURRENT]);
        }

        if (got->requests[k][(int) PENDING] != 
            (int)(expected->requests[k][(int) PENDING]))
        {
            CDR_MSG(0, "\t%s: FINAL RESPONSE pending request[%d] failure\n", SELF, k);
            CDR_MSG(0, "\t%s: got  %d, expected %d\n", 
                    SELF, 
                    got->requests[k][(int) PENDING], 
                    expected->requests[k][(int) PENDING]);
        }
    }
}


/*
 * Name:
 *
 *        st_show_acs()
 *
 * Description:
 *
 *        This function prints acs information from the simulated server for 
 *        AUDIT ACS requests made by main. It reports the acs' id & status.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth acs being reported.
 *        stat_got - STATUS returned from the server.
 *        acs_got - ptr to acs information returned from server.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_acs"
void st_show_acs(unsigned int cnt, 
                 STATUS       stat_got, 
                 ACS          acs_got)
{
    CDR_MSG(4, "\t%s: FINAL RESPONSE acs[%d] id = %d\n", SELF, cnt, acs_got );
    CDR_MSG(4, "\t%s: acs[%d] status is %s\n", SELF, cnt, acs_status(stat_got));
}  


/*
 * Name:
 *
 *        st_chk_acs()
 *
 * Description:
 *
 *        This function prints acs information from the simulated server for 
 *        AUDIT ACS requests made by main. It reports the acs' id & status.
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth acs being reported.
 *        stat_got - STATUS returned from the server.
 *        stat_exp - STATUS expected.
 *        acs_got - ptr to acs information returned from server.
 *        acs_exp - ptr to acs information expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_acs"
void st_chk_acs(unsigned int cnt, 
                STATUS       stat_got, 
                STATUS       stat_exp, 
                ACS          acs_got, 
                ACS          acs_exp)
{
    if (acs_got != acs_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE acs[%d] id failure\n", SELF, cnt);
        CDR_MSG(0, "\t    got %d, expected %d\n", acs_got, acs_exp);
    }

    if (stat_got != stat_exp)
    {
        CDR_MSG(0, "\t%s: %s acs[%d] status failure\n", SELF, fin_resp_str, cnt);
        CDR_MSG(0, "\t          got %s, expected %s\n", 
                acs_status(stat_got), 
                acs_status(stat_exp));
    }
}


/*
 * Name:
 *
 *        st_chk_acs_status()
 *
 * Description:
 *
 *        This function prints acs information from the simulated server for 
 *        QUERY ACS requests made by main. It reports the acs' id, its state,
 *        its free cells, its status, and the status of its request.
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth acs being reported.
 *        sp - ptr to acs information returned from the server.
 *        acs_id - ptr to acs information expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_acs_status"
void st_chk_acs_status(unsigned int            cnt, 
                       ACS_QUERY_ACS_RESPONSE *sp, 
                       ACS                    *acs_id)
{
    unsigned int    i, j, k;
    REQ_SUMMARY     req;

    for (j = k = 0; k < 5; k++, j++)
    {
        req.requests[k][0] = j;
        req.requests[k][1] = j + 1;
    }

    if (cnt != sp->count)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE acs count failure\n", SELF);
        CDR_MSG(0, "\t    got %d, expected %d\n", sp->count, cnt);
    }
    else
    {
        for (i = 0; i < cnt; i++)
        {
            if (sp->acs_status[i].acs_id != *(acs_id + i))
            {
                CDR_MSG(0, "\t%s: FINAL RESPONSE acs id failure\n", SELF);
                CDR_MSG(0, "\t    got %d, expected %d\n", 
                        sp->acs_status[i].acs_id, 
                        *(acs_id + 1));
            }

            st_chk_state(i, 
                         sp->acs_status[i].state, 
                         STATE_OFFLINE_PENDING);

            st_chk_freecells(i, 
                             sp->acs_status[i].freecells, 
                             100);

            if (sp->acs_status[i].status != STATUS_SUCCESS)
            {
                CDR_MSG(0, "\t%s: %s acs[%d] status failure\n", SELF, fin_resp_str, i);
                CDR_MSG(0, "\t          got %s, expected %s\n", 
                        acs_status(sp->acs_status[i].status), 
                        acs_status(STATUS_SUCCESS));
            }

            st_chk_requests(&(sp->acs_status[i].requests), 
                            &req);
        }
    }
}


#undef SELF
#define SELF "st_cap_mode"
char *st_cap_mode(CAP_MODE cm)
{
    switch (cm)
    {
    case CAP_MODE_AUTOMATIC:
        sprintf(str, "CAP_MODE_AUTOMATIC");
        break;

    case CAP_MODE_MANUAL:
        sprintf(str, "CAP_MODE_MANUAL");
        break;

    case CAP_MODE_SAME:
        sprintf(str, "CAP_MODE_SAME");
        break;

    case CAP_MODE_FIRST:
        sprintf(str, "CAP_MODE_FIRST");
        break;

    case CAP_MODE_LAST:
        sprintf(str, "CAP_MODE_LAST");
        break;

    default:
        sprintf(str, "CAP_MODE_UNKNOWN");
    }

    return str;
}


#undef SELF
#define SELF "st_cap_priority"
char *st_cap_priority(CAP_PRIORITY cp)
{
    if (cp == NO_PRIORITY)
    {
        sprintf(str, "NO_PRIORITY");
    }
    else if (cp == MIN_PRIORITY)
    {
        sprintf(str, "MIN_PRIORITY");
    }
    else if (cp == MAX_PRIORITY)
    {
        sprintf(str, "MAX_PRIORITY");
    }
    else if (cp == SAME_PRIORITY)
    {
        sprintf(str, "SAME_PRIORITY");
    }
    else
    {
        sprintf(str, "PRIORITY_%d", cp);
    }

    return str;
}


/*
 * Name:
 *
 *        st_show_cap_requested()
 *
 * Description:
 *
 *        This function prints cap data from the server for the EJECT 
 *        requests made by main. This data is for the cap requested,
 *        which is not necessarily the cap used. It only reports the values, it does no checking..
 *
 * Explicit Inputs:
 *        
 *        cap_id - pointer to the cap info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_cap_requested"
void st_show_cap_requested(CAPID *cap_id)
{
    CDR_MSG(4, "\t%s cap requested is (%d,%d,%d)\n", 
            SELF, 
            cap_id->lsm_id.acs,
            cap_id->lsm_id.lsm, 
            cap_id->cap);
}


/*
 * Name:
 *
 *        st_show_cap_info()
 *
 * Description:
 *
 *        This function prints cap data from the server for ENTER, VARY CAP,  
 *        QUERY CAP, EJECT, and SET CAP requests made by main. 
 *        The data is for the cap actually used. This function only reports 
 *        the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth cap being reported.
 *        stat - pointer to the status of the nth cap.
 *        cap_id - pointer to the cap information structure for the nth cap.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_cap_info"
void st_show_cap_info(int     i, 
                      STATUS *stat, 
                      CAPID  *cap_id)
{
    if (stat)
    {
        CDR_MSG(4, "\t%s: cap[%d]: status is %s\n", SELF, i, acs_status(*stat));
    }

    if (i >= 0)
    {
        CDR_MSG(4, "\t%s: cap[%d]  capid = (%d,%d,%d)\n", 
                SELF, i, cap_id->lsm_id.acs, cap_id->lsm_id.lsm, cap_id->cap);
    }
    else
    {
        CDR_MSG(4, "\t%s: cap for the current volume - capid = (%d,%d,%d)\n", 
                SELF, cap_id->lsm_id.acs, cap_id->lsm_id.lsm, cap_id->cap);
    }
}


/*
 * Name:
 *
 *        st_chk_cap_info()
 *
 * Description:
 *
 *        This function prints cap data from the server for ENTER, 
 *        VARY CAP,  QUERY CAP, EJECT, and SET CAP requests made by main. 
 *        The data is for the cap actually used. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        cnt1 - indicates the volume count in the cap returned from server.
 *        cnt2 - indicates the volume count in the cap expected.
 *        stat_got- the status of the cap returned from server.
 *        stat_exp- the status of the cap expected.
 *        got - pointer to cap info structure returned from server.
 *        expected - pointer to cap info structure expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_cap_info"
void st_chk_cap_info(unsigned short  cnt1, 
                     unsigned short  cnt2, 
                     STATUS         *stat_got, 
                     STATUS          stat_exp, 
                     CAPID          *got, 
                     CAPID          *expected)
{
    if (stat_got)
    {
        if (*stat_got != stat_exp)
        {
            CDR_MSG(0, "\t%s: %s cap_id[%d] status failure\n", SELF, fin_resp_str, cnt1);
            CDR_MSG(0, "\t          got %s, expected %s\n", 
                    acs_status(*stat_got), 
                    acs_status(stat_exp));
        }
    }

    st_chk_lsm_id(0, 
                  NULL, 
                  STATUS_SUCCESS, 
                  &(got->lsm_id), 
                  &(expected->lsm_id));

    if (got->cap != expected->cap)
    {
        CDR_MSG(0, "\t%s:  INTERMEDIATE RESPONSE cap failure\n", SELF);
        CDR_MSG(0, "\t    got %d, expected %d\n", got->cap, expected->cap);
    }

    if (cnt1 != cnt2)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE vol count failure\n", SELF);
        CDR_MSG(0, "\t    got %d, expected %d\n", cnt1, cnt2);
    }
}


/*
 * Name:
 *
 *        st_show_cap_misc()
 *
 * Description:
 *
 *        This function prints additional cap data from the server for 
 *        QUERY CAP, and SET CAP requests made by main. 
 *        The data is for the cap actually used. This function only reports 
 *        the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth cap being reported.
 *        cs - pointer to the state of the cap.
 *        cm - pointer to the mode of the cap.
 *        cp - pointer to the priority of the cap.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_cap_misc"
void st_show_cap_misc(unsigned short  i, 
                      STATE          *cs, 
                      CAP_MODE        cm, 
                      CAP_PRIORITY    cp)
{
    if (cs)
    {
        st_show_state(i, *cs);
    }

    CDR_MSG(4, "\t%s: cap[%d] cap_priority = %s\n", SELF, i, st_cap_priority(cp));
    CDR_MSG(4, "\t%s: cap[%d] cap_mode = %s\n", SELF, i, st_cap_mode(cm));
}


/*
 * Name:
 *
 *        st_chk_cap_misc()
 *
 * Description:
 *
 *        This function prints additional cap data from the simulated server
 *        for QUERY CAP and SET CAP requests made by main. The data is for
 *        the cap actually used.
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth cap being reported.
 *        cs_got - pointer to the state of the cap returned by t_acslm.
 *        cs_exp - pointer to the state of the cap expected.
 *        cm_got - pointer to the mode of the cap returned by t_acslm.
 *        cm_exp - pointer to the mode of the cap expected.
 *        cp_got - pointer to the priority of the cap returned by t_acslm.
 *        cp_exp - pointer to the priority of the cap expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_cap_misc"
void st_chk_cap_misc(unsigned short  i, 
                     STATE          *cs_got, 
                     STATE           cs_exp, 
                     CAP_MODE        cm_got, 
                     CAP_MODE        cm_exp, 
                     CAP_PRIORITY    cp_got, 
                     CAP_PRIORITY    cp_exp)
{
    if (cs_got)
    {
        st_chk_state(i, 
                     *cs_got, 
                     cs_exp);
    }

    if (cp_got != cp_exp)
    {
        CDR_MSG(0, "\t%s: cap[%d] priority failure\n", SELF, i);
        CDR_MSG(0, "\t    expected %s, got %s\n", 
                st_cap_priority(cp_exp), st_cap_priority(cp_got));
    }

    if (cm_got != cm_exp)
    {
        CDR_MSG(0, "\t%s: cap[%d] mode failure\n", SELF, i);
        CDR_MSG(0, "\t    expected %s, got %s\n", 
                st_cap_mode(cm_exp), st_cap_mode(cm_got));
    }
}


/*
 * Name:
 *
 *        st_show_lsm_id()
 *
 * Description:
 *
 *        This function prints lsm data from the server for QUERY LSM,
 *        AUDIT LSM, and VARY LSM requests made by main. It also is called
 *        by st_show_panel_info(). This function only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth lsm being reported.
 *        lsm_status - pointer to the status of the nth lsm.
 *        lsm_id - pointer to the info structure for the nth lsm.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_lsm_id"
void st_show_lsm_id(unsigned short  i, 
                    STATUS         *lsm_status, 
                    LSMID          *lsm_id)
{
    if (lsm_status)
    {
        CDR_MSG(4, "\t%s: lsm[%d]: status is %s\n", 
                SELF, i, acs_status(*lsm_status));
    }

    CDR_MSG(4, "\t%s: acs[%d] is %d\n", SELF, i, lsm_id->acs);
    CDR_MSG(4, "\t%s: lsm id[%d] is %d\n", SELF, i, lsm_id->lsm);
}


/*
 * Name:
 *
 *        st_chk_lsm_id()
 *
 * Description:
 *
 *        This function prints lsm data from the simulated server for 
 *        QUERY LSM, AUDIT LSM, and VARY LSM requests made by main. 
 *        It is also called by st_chk_panel_info().
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth lsm being reported.
 *        stat_got - pointer to the status of the nth lsm 
 *                   returned by the server.
 *        stat_expected - pointer to the expected status of the nth lsm.
 *        got - pointer to the info structure of the nth lsm returned
 *              by the server.
 *        expected - pointer to the expected information for the nth lsm.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_lsm_id"
void st_chk_lsm_id(unsigned short  i, 
                   STATUS         *stat_got, 
                   STATUS          stat_expected, 
                   LSMID          *got, 
                   LSMID          *expected)
{
    if (stat_got)
    {
        if (*stat_got != stat_expected)
        {
            CDR_MSG(0, "\t%s: %s lsm[%d] status failure\n", SELF, fin_resp_str, i);
            CDR_MSG(0, "\t          got %s, expected %s\n", 
                    acs_status(*stat_got), 
                    acs_status(stat_expected));
        }
    }

    if (got->acs != expected->acs)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE lsm [%d] acs failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                got->acs, expected->acs);
    }

    if (got->lsm != expected->lsm)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE lsm [%d] lsm failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                got->lsm, expected->lsm);
    }
}


/*
 * Name:
 *
 *        st_show_pool_misc()
 *
 * Description:
 *
 *        This function prints pool data from the server for QUERY and
 *        DEFINE POOL requests made by main. 
 *        This function  only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        lwm - the low water mark value of the pool.
 *        hwm - the high water mark value of the pool.
 *        attributes -  the characteristics of the pool. 
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_pool_misc"
void st_show_pool_misc(int           cnt,
                       unsigned long lwm, 
                       unsigned long hwm, 
                       unsigned long attributes)
{
    CDR_MSG(4, "\t%s: low water mark is %d\n", SELF, lwm);
    CDR_MSG(4, "\t%s: high water mark is %d\n", SELF, hwm);
    CDR_MSG(4, "\t%s: attributes are %s\n", 
            SELF, ((attributes) == OVERFLOW ? "OVERFLOW" : "UNKNOWN"));
}


/*
 * Name:
 *
 *        st_chk_pool_misc()
 *
 * Description:
 *
 *        This function prints pool data from the simulated server for 
 *        QUERY and DEFINE POOL requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        lwm_got - the low water mark value of the pool returned by t_acslm.
 *        lwm_exp - the low water mark value of the pool expected.
 *        hwm_got - the high water mark value of the pool returned by t_acslm.
 *        hwm_exp - the high water mark value of the pool expected.
 *        attrs_got -  the characteristics of the pool returned by t_acslm. 
 *        attrs_exp -  the characteristics of the pool expected. 
 *
 * Return Values:
 *
 *        None.
 */

#undef SELF
#define SELF "st_chk_pool_misc"

void st_chk_pool_misc(unsigned long lwm_got, 
                      unsigned long lwm_exp, 
                      unsigned long hwm_got, 
                      unsigned long hwm_exp, 
                      unsigned long attrs_got, 
                      unsigned long attrs_exp)
{
    if (lwm_got != lwm_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE lwm failure\n", SELF);
        CDR_MSG(0, "\t           got %d, expected %d\n", lwm_got, lwm_exp);
    }

    if (hwm_got != hwm_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE hwm failure\n", SELF);
        CDR_MSG(0, "\t           got %d, expected %d\n", hwm_got, hwm_exp);
    }

    if (attrs_got != attrs_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE attributes failure\n", SELF);
        CDR_MSG(0, "\t           got %s, expected %s\n", 
                (attrs_got) == OVERFLOW
                ? "OVERFLOW" : "UNKNOWN", 
                (attrs_exp) == OVERFLOW
                ? "OVERFLOW" : "UNKNOWN");
    }
}


/*
 * Name:
 *
 *        st_show_pool_info()
 *
 * Description:
 *
 *        This function prints pool data from the server for QUERY POOL,
 *        QUERY SCRATCH, QUERY MOUNT SCRATCH, MOUNT SCRATCH, DELETE POOL, 
 *        and DEFINE POOL requests made by main. 
 *        This function  only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        cnt - the nth pool.
 *        s_ptr - pointer to the status of the nth pool.
 *        p_ptr - pointer to the pool id of the nth pool.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_pool_info"
void st_show_pool_info(unsigned short  cnt, 
                       STATUS         *s_ptr, 
                       POOL           *p_ptr)
{
    CDR_MSG(4, "\t%s: pool[%d] id of %d\n", SELF, cnt, *p_ptr);

    if (s_ptr)
    {
        CDR_MSG(4, "\t%s: pool[%d] has status of %s\n", 
                SELF, cnt, acs_status(*s_ptr));
    }
}


/*
 * Name:
 *
 *        st_chk_pool_info()
 *
 * Description:
 *
 *        This function prints pool data from the simulated server for 
 *        QUERY POOL, QUERY MOUNT SCRATCH, QUERY MOUNT, MOUNT SCRATCH, 
 *        DELETE POOL and DEFINE POOL requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth pool being reported.
 *        status_got - the state of the pool returned from the server.
 *        stat_expected - the state of the pool expected.
 *        pool_got - pointer to the identifier of the nth pool returned.
 *        pool_expected - pointer to the expeect identifier of the nth pool.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_pool_info"
void st_chk_pool_info(unsigned short  cnt, 
                      POOL           *pool_got, 
                      STATUS         *status_got, 
                      POOL           *pool_expected, 
                      STATUS          stat_expected)
{
    if (*pool_got != *pool_expected)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE pool[%d] failure\n", SELF, cnt);
    }
    if ((status_got) && (*status_got != stat_expected))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE pool_status[%d] failure\n", SELF, cnt);
    }
}


/*
 * Name:
 *
 *        st_show_port_info()
 *
 * Description:
 *
 *        This function prints port info from the server for QUERY and
 *        VARY PORT requests made by main. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth port being reported.
 *        pid_got - pointer to the port info structure.
 *        state - the state of the port being reported on.
 *        status - the status of the port being reported on.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_port_info"

void st_show_port_info(unsigned short  i, 
                       PORTID         *pid_got, 
                       STATE           state, 
                       STATUS          stat)
{
    CDR_MSG(4, "\t%s: port[%d]: status is %s\n", SELF, i, 
            acs_status(stat));
    CDR_MSG(4, "\t%s:  state[%d] is %s\n", SELF, i, 
            acs_state(state));
    CDR_MSG(4, "\t%s:  acs[%d] is %d\n", SELF, i, 
            pid_got->acs);
    CDR_MSG(4, "\t%s:  port id[%d] is %d\n", SELF, i, 
            pid_got->port);
}


/*
 * Name:
 *
 *        st_chk_port_info()
 *
 * Description:
 *
 *        This function prints port data from the simulated server for 
 *        QUERY and VARY PORT requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth port being reported.
 *        pid_got - pointer to the port info returned by t_acslm.
 *        pid_got - pointer to the port info expected.
 *        state_got - the state of the port being reported on.
 *        state_got - the expected state of the port.
 *        status_got - the status of the port being reported on.
 *        status_exp - the expected status of the port.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_port_info"
void st_chk_port_info(unsigned short  i, 
                      PORTID         *pid_got, 
                      PORTID         *pid_exp, 
                      STATE           state_got, 
                      STATE           state_exp, 
                      STATUS          status_got, 
                      STATUS          status_exp)
{
    if (pid_got->acs != pid_exp->acs)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE acs failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]:got %d, expected %d\n", 
                SELF, i, pid_got->acs, pid_exp->acs);
    }

    if (pid_got->port != pid_exp->port)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE port failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]: got %d, expected %d\n", 
                SELF, i, pid_got->port, pid_exp->port);
    }

    if (state_got != state_exp)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE state failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]:    got %s, expected %s\n", 
                SELF, i, acs_state(state_got), acs_state(state_exp));
    }

    if (status_got != status_exp)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE status failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]:    expected %s, got %s\n", 
                SELF, i, acs_status(status_got), acs_status(status_exp));
    }
}


/*
 * Name:
 *
 *        st_show_req_status()
 *
 * Description:
 *
 *        This function prints outstanding request data from the server for 
 *        QUERY REQUEST requests made by main. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth request being reported.
 *        sp - pointer to the request info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_req_status"
void st_show_req_status(unsigned short  i, 
                        QU_REQ_STATUS  *sp)
{
    CDR_MSG(4, "\t%s: request[%d]: status is %s\n", 
            SELF, i, acs_status(sp->status));
    CDR_MSG(4, "\t%s:  command[%d] is %s\n", 
            SELF, i, acs_command(sp->command));
    CDR_MSG(4, "\t%s:  request id[%d] is %d\n", 
            SELF, i, sp->request);
}


/*
 * Name:
 *
 *        st_chk_req_status()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        QUERY REQUEST requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth request being reported.
 *        sp_got - pointer to the request information returned from the server.
 *        sp_expected - pointer to the request information expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_req_status"
void st_chk_req_status(unsigned short  i, 
                       QU_REQ_STATUS  *sp_got, 
                       QU_REQ_STATUS  *sp_exp)
{
    if (sp_got->request != sp_exp->request)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE id failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]: got %d, expected %d\n", 
                SELF, i, sp_got->request, sp_exp->request);
    }

    if (sp_got->command != sp_exp->command)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE state failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]:    got %s, expected %s\n", 
                SELF, 
                i, 
                acs_command(sp_got->command), 
                acs_command(sp_exp->command));
    }

    if (sp_got->status != sp_got->status)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE status failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]:    expected %s, got %s\n", 
                SELF, 
                i, 
                acs_status(sp_got->status), 
                acs_status(sp_exp->status));
    }
}


/*
 * Name:
 *
 *        st_chk_lock_info()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        several QUERY and AUDIT requests made by main. It is used to 
 *        print state info for lsms, acses, and drives. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        state_got - the state of the device returned from the server.
 *        state_expected - the state of the device expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_lock_info"
void st_chk_lock_info(unsigned short i, 
                      LOCKID         lock_got, 
                      LOCKID         lock_exp, 
                      unsigned long  ld_got, 
                      unsigned long  ld_exp, 
                      unsigned long  lp_got, 
                      unsigned long  lp_exp, 
                      USERID        *ui_got, 
                      USERID        *ui_exp)
{
    if (lock_got != lock_exp)
    {
        CDR_MSG(0, "\t%s[%d]: FINAL RESPONSE lock_id failure\n", SELF, i);
        CDR_MSG(0, "\t%s[%d]: got %d, expected %d\n", 
                SELF, i, lock_got, lock_exp);
    }

    if (ld_got != ld_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE lock_duration failure\n", SELF);
        CDR_MSG(0, "\t%s[%d]: got %d, expected %d\n", 
                SELF, i, ld_got, ld_exp);
    }

    if (lp_got != lp_exp)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE locks_pending failure\n", SELF);
        CDR_MSG(0, "\t%s[%d]: got %d, expected %d\n", 
                SELF, i, lp_got, lp_exp);
    }

    if (strncmp(ui_got->user_label, ui_exp->user_label, EXTERNAL_LABEL_SIZE))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE user_id failure\n", SELF);
        CDR_MSG(0, "\t%s[%d]:    got %s, expected %s\n", 
                SELF, i, ui_got->user_label, ui_exp->user_label);
    }
}


/*
 * Name:
 *
 *        st_show_lock_info()
 *
 * Description:
 *
 *        This function prints state data from the server for QUERY 
 *        LOCK DRIVE and QUERY LOCK VOLUME requests made by main. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth lock being reported.
 *        lock_id - the identifier for the lock.
 *        lock_dur - the duration of the lock, in seconds.
 *        locks_pend - number of other requests for locking the resource.
 *        user - user identifier associated with the lock.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_lock_info"
void st_show_lock_info(unsigned short i, 
                       LOCKID         lock_id, 
                       unsigned long  lock_dur, 
                       unsigned long  locks_pend, 
                       USERID        *user)
{
    CDR_MSG(4, "\t%s: lock_id[%d] is %d\n", SELF, i, lock_id);
    CDR_MSG(4, "\t%s:     lock duration is %d\n", SELF, lock_dur);
    CDR_MSG(4, "\t%s:     locks pending are %d\n", SELF, locks_pend);
    CDR_MSG(4, "\t%s:     user id is  %s\n", SELF, user->user_label);
}


/*
 * Name:
 *
 *        st_show_lo_vol_status()
 *
 * Description:
 *
 *        This function prints information from the server for  
 *        CLEAR/UNLOCK/LOCK VOLUME requests made by main. 
 *        It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth volume being reported.
 *        lvs_ptr - pointer to the locked volume's info structure.
 *        vol_id - expected volume info if in checking mode.
 *
 * Implicit Inputs:
 *        check_mode - Boolean indicating native server if false.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_low_vol_status"
void st_show_lo_vol_status(unsigned short     cnt, 
                           ACS_LO_VOL_STATUS *lvs_ptr, 
                           VOLID             *vol_id,
                           char              *callingRoutine)
{
    unsigned short  i;
    ACS_LO_VOL_STATUS *tmpptr;
    VOLID       *tmpid;

    /* a native server, so just report values */
    if (!check_mode)
    {
        tmpptr = lvs_ptr;

        for (i = 0; i < cnt; i++)
        {
            st_show_vol_info(i, 
                             &(tmpptr->status), 
                             &(tmpptr->vol_id));

            if (tmpptr->status == STATUS_DEADLOCK)
            {
                CDR_MSG(0, "\t%s: deadlocked volume[%d] has external label of %s\n", 
                        SELF, i, &(tmpptr->dlocked_vol_id.external_label[0]));
            }

            tmpptr++;
        }
    }
    /* the Toolkit simulated server, so check against expected */
    else
    {
        tmpptr = lvs_ptr;
        tmpid = vol_id;

        for (i = 0; i < cnt; i++)
        {
            st_chk_vol_info(i, &(tmpptr->status), 
                            (i == 0 && !strcmp(callingRoutine, "c_lock_volume"))
                            ? STATUS_DEADLOCK
                            : STATUS_SUCCESS, 
                            &(tmpptr->vol_id), tmpid);
            tmpptr++;
            tmpid++;
        }
    }
}


/*
 * Name:
 *
 *        st_show_vol_info()
 *
 * Description:
 *
 *        This function prints volume info from the server for several QUERY,
 *        ENTER, and SET requests made by main. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth volume being reported.
 *        stat - the status of the nth volume.
 *        from_server - pointer to the volume info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_vol_info"
void st_show_vol_info(unsigned short  cnt, 
                      STATUS         *stat, 
                      VOLID          *from_server)
{
    if (stat)
    {
        CDR_MSG(4, "\t%s: volume[%d] has status of %s\n", 
                SELF, cnt, acs_status(*stat));
    }

    CDR_MSG(4, "\t%s: volume[%d] has external label of %s\n", 
            SELF, cnt, &(from_server->external_label[0]));
}


/*
 * Name:
 *
 *        st_chk_vol_info()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        several QUERY and AUDIT requests made by main. It is used to 
 *        print state info for lsms, acses, and drives. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        state_got - the state of the device returned from the server.
 *        state_expected - the state of the device expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_vol_info"
void st_chk_vol_info(unsigned short  i, 
                     STATUS         *stat_got, 
                     STATUS          stat_expected, 
                     VOLID          *got, 
                     VOLID          *expected)
{
    if (stat_got)
    {
        if (*stat_got != stat_expected)
        {
            CDR_MSG(0, "\t%s: %s vol_id[%d] status failure\n", SELF, fin_resp_str, i);
            CDR_MSG(0, "\t          got %s, expected %s\n", 
                    acs_status(*stat_got), acs_status(stat_expected));
        }
    }

    if (strncmp(got->external_label, expected->external_label, EXTERNAL_LABEL_SIZE) != 0)
    {
        CDR_MSG(0, "\t%s: %s vol_id[%d] label failure\n", SELF, fin_resp_str, i);
        CDR_MSG(0, "\t          got %s, expected %s\n", 
                got->external_label, 
                expected->external_label);
    }
}


/*
 * Name:
 *
 *        st_show_lo_drv_status()
 *
 * Description:
 *
 *        This function prints information from the server for  
 *        CLEAR/UNLOCK/LOCK DRIVE requests made by main. 
 *        It has two modes of operation.
 *        - a checking mode when interacting with the sample server t_acslm.
 *        - a reporting mode when interacting with a native server.
 *
 * Explicit Inputs:
 *        
 *        cnt - indicates the nth drive being reported.
 *        lds_ptr - pointer to a structure containing drive information.
 *        drive_id - pointer to the identifier of the drive.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_lo_drv_status"
void st_show_lo_drv_status(unsigned short     cnt, 
                           ACS_LO_DRV_STATUS *lds_ptr, 
                           DRIVEID           *drive_id,
                           char              *callingRoutine)
{
    unsigned short  i;
    ACS_LO_DRV_STATUS *tmpptr;
    DRIVEID     *tmpid;

    if (!check_mode)
    {
        tmpptr = lds_ptr;

        for (i = 0; i < cnt; i++)
        {
            st_show_drv_info(i, 
                             &(tmpptr->status), 
                             &(tmpptr->drive_id));

            if (tmpptr->status == STATUS_DEADLOCK)
            {
                CDR_MSG(0, "\t%s: deadlocked drive[%d] has drive id of %d\n", 
                        SELF, i, tmpptr->dlocked_drive_id.drive);
            }

            tmpptr++;
        }
    }
    else
    {
        tmpptr = lds_ptr;
        tmpid = drive_id;

        for (i = 0; i < cnt; i++)
        {
            st_chk_drv_info(i, &(tmpptr->status), 
                            (i == 0 && !strcmp(callingRoutine, "c_lock_drive"))
                            ? STATUS_DEADLOCK
                            : STATUS_SUCCESS, 
                            (&tmpptr->drive_id), 
                            tmpid);
            tmpptr++;
            tmpid++;
        }
    }
}


/*
 * Name:
 *
 *        st_show_drv_info()
 *
 * Description:
 *
 *        This function prints volume info from the server for several QUERY,
 *        ENTER, and SET requests made by main. 
 *        It only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive being reported.
 *        status - pointer to the status of the drive.
 *        from_server - pointer to the drive's identifier.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_drv_info"
void st_show_drv_info(unsigned short  i, 
                      STATUS         *stat, 
                      DRIVEID        *from_server)
{
    if (stat)
    {
        CDR_MSG(4, "\t%s: drive[%d] has status of %s\n", 
                SELF, i, acs_status(*stat));
    }

    CDR_MSG(4, "\t%s: drive[%d] has drive id of %d\n", 
            SELF, i, from_server->drive);

    st_show_panel_info(i, NULL, &(from_server->panel_id));
}


/*
 * Name:
 *
 *        st_show_drv_status()
 *
 * Description:
 *
 *        This function prints information from the server for several QUERY 
 *        requests made by main. 
 *        This function only reports the values, it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth drive being reported.
 *        sp - pointer to the structure containing the drive's info.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_drv_status"
void st_show_drv_status(unsigned short  i, 
                        QU_DRV_STATUS  *sp)
{
    st_show_drv_info(i, &(sp->status), &(sp->drive_id));

    /* If the drive is in use, show the volume's info */
    if (sp->status == STATUS_DRIVE_IN_USE)
    {
        st_show_vol_info(i, NULL, &(sp->vol_id));
    }

    st_show_state(i, sp->state);
    st_show_drive_type(i, sp->drive_type);
}


/*
 * Name:
 *
 *        st_chk_drv_status()
 *
 * Description:
 *
 *        This function prints state data from the simulated server for 
 *        several QUERY and AUDIT requests made by main. It is used to 
 *        print state info for lsms, acses, and drives. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        state_got - the state of the device returned from the server.
 *        state_expected - the state of the device expected.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_drv_status"
void st_chk_drv_status(unsigned short  i, 
                       QU_DRV_STATUS  *sp, 
                       QU_DRV_STATUS  *sp_expected)
{
    st_chk_drv_info(i, 
                    &(sp->status), 
                    sp_expected->status, 
                    &(sp->drive_id), 
                    &sp_expected->drive_id);

    st_chk_state(i, 
                 sp->state, 
                 sp_expected->state);

    st_chk_drive_type(i, 
                      sp->drive_type, 
                      sp_expected->drive_type);

    if (sp->status == STATUS_DRIVE_IN_USE)
    {
        st_chk_vol_info(i, 
                        NULL, 
                        STATUS_SUCCESS, 
                        &(sp->vol_id), 
                        &(sp_expected->vol_id));
    }
}


/*
 * Name:
 *
 *        st_show_panel_info()
 *
 * Description:
 *
 *        This function prints data from the server for AUDIT PANEL
 *        requests made by main. It is also called by st_show_drive_info()
 *        and st_show_cellid(). It only reports the values, 
 *        it does no checking.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth panel being reported.
 *        stat - pointer to the status of the panel.
 *        from_server - pointer to the panel info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_panel_info"
void st_show_panel_info(unsigned short  i, 
                        STATUS         *stat, 
                        PANELID        *from_server)
{
    if (stat)
    {
        CDR_MSG(4, "\t%s: panel[%d] status is %s\n", SELF, i, acs_status(*stat));
    }

    CDR_MSG(4, "\t%s: panel [%d] is  %d\n", SELF, i, from_server->panel);

    st_show_lsm_id(i, NULL, &from_server->lsm_id);
}


/*
 * Name:
 *
 *        st_chk_panel_info()
 *
 * Description:
 *
 *        This function prints data from the simulated server for 
 *        AUDIT PANEL requests made by main. It is also called by 
 *        st_chk_drive_info() and st_chk_cellid().
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth panel being reported.
 *        stat_got - pointer to the status of the panel returned from t_acslm.
 *        stat_expected - the expected status of the panel.
 *        got - pointer to panel information returned from t_acslm.
 *        expected - pointer to expected panel information.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_panel_info"
void st_chk_panel_info(unsigned short  i, 
                       STATUS         *stat_got, 
                       STATUS          stat_exp, 
                       PANELID        *got, 
                       PANELID        *expected)
{
    if ((stat_got) && (*stat_got != stat_exp))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE panel[%d] status failure\n", SELF, i);
        CDR_MSG(0, "\t    got %s, expected %s\n", 
                acs_status(*stat_got), acs_status(stat_exp));
    }

    st_chk_lsm_id(i, 
                  NULL, 
                  STATUS_SUCCESS, 
                  &got->lsm_id, 
                  &expected->lsm_id);
}


/*
 * Name:
 *
 *        st_chk_drv_info()
 *
 * Description:
 *
 *        This function prints volume info from the server for several QUERY,
 *        ENTER, and SET requests made by main. 
 *        It checks the values returned from t_acslm and compares them
 *        against expected values. It prints only if there is a difference.
 *
 * Explicit Inputs:
 *        
 *        i - indicates the nth acs, lsm, or drive being reported.
 *        stat_got - pointer to the drive status returned from t_acslm.
 *        stat_expected - the drive status expected.
 *        from_server - pointer to the drive information from t_acslm.
 *        expected - pointer to the expected drive information.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_chk_drv_info"
void st_chk_drv_info(unsigned short  i, 
                     STATUS         *stat_got, 
                     STATUS          stat_expected, 
                     DRIVEID        *from_server, 
                     DRIVEID        *expected)
{
    if ((stat_got) && (*stat_got != stat_expected))
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive[%d] status failure\n", SELF, i);
        CDR_MSG(0, "\t    got %s, expected %s\n", 
                acs_status(*stat_got), acs_status(stat_expected));
    }

    if (from_server->panel_id.lsm_id.acs != 
        expected->panel_id.lsm_id.acs)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive[%d] acs failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                from_server->panel_id.lsm_id.acs, 
                expected->panel_id.lsm_id.acs);
    }

    if (from_server->panel_id.lsm_id.lsm != 
        expected->panel_id.lsm_id.lsm)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive[%d] lsm failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                from_server->panel_id.lsm_id.lsm, 
                expected->panel_id.lsm_id.lsm);
    }

    if (from_server->panel_id.panel != 
        expected->panel_id.panel)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive[%d] panel failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                from_server->panel_id.panel, 
                expected->panel_id.panel);
    }

    if (from_server->drive != expected->drive)
    {
        CDR_MSG(0, "\t%s: FINAL RESPONSE drive[%d] drive failure\n", SELF, i);
        CDR_MSG(0, "\t    got %d, expected %d\n", 
                from_server->drive, expected->drive);
    }
}


#undef SELF
#define SELF "st_reqtostr"
char *st_reqtostr(int k)
{
    static char     tbuf[80];

    switch (k)
    {
    case 0:
        strcpy(tbuf, "   AUDIT");
        break;

    case 1:
        strcpy(tbuf, "   MOUNT");
        break;

    case 2:
        strcpy(tbuf, "DISMOUNT");
        break;

    case 3:
        strcpy(tbuf, "   ENTER");
        break;

    case 4:
        strcpy(tbuf, "   EJECT");
        break;

    default:
        sprintf(tbuf, "Unknown: %d", k);
    }

    return tbuf;
}


#undef SELF
#define SELF "st_rttostr"
char *st_rttostr(ACS_RESPONSE_TYPE k)
{
    static char     tbuf[80];

    switch (k)
    {
    case RT_ACKNOWLEDGE:
        strcpy(tbuf, "RT_ACKNOWLEDGE");
        break;

    case RT_FINAL:
        strcpy(tbuf, "RT_FINAL");
        break;

    case RT_INTERMEDIATE:
        strcpy(tbuf, "RT_INTERMEDIATE");
        break;

    default:
        sprintf(tbuf, "RT_Unknown: %d", k);
    }

    return tbuf;
}


#undef SELF
#define SELF "decode_mopts"
char *decode_mopts(unsigned char msgopt)
{
    extern char     str_buffer[];

    strcpy(str_buffer, "<");

    if (msgopt == '\0')
    {
        strcpy(str_buffer, "<NONE>");

        return(str_buffer);
    }

    if (msgopt & FORCE)
    {
        strcat(str_buffer, "FORCE|");
    }

    if (msgopt & INTERMEDIATE)
    {
        strcat(str_buffer, "INTERMEDIATE|");
    }

    if (msgopt & ACKNOWLEDGE)
    {
        strcat(str_buffer, "ACKNOWLEDGE|");
    }

    if (msgopt & READONLY)
    {
        strcat(str_buffer, "READONLY|");
    }

    if (msgopt & EXTENDED)
    {
        strcat(str_buffer, "EXTENDED|");
    }

    /* Check for bad value (none of the above) */
    if (strlen(str_buffer) == 1)
    {
        strcat(str_buffer, "BAD ");
    }

    /* terminate the string */
    str_buffer[strlen(str_buffer) - 1] = '>';

    return(str_buffer);
}


#undef SELF
#define SELF "decode_vers"
char *decode_vers(long vers)
{
    static char     buffer[60];

    switch (vers)
    {
    case VERSION0:
        strcpy(buffer, "<VERSION0>");
        break;

    case VERSION1:
        strcpy(buffer, "<VERSION1>");
        break;

    case VERSION2:
        strcpy(buffer, "<VERSION2>");
        break;

    case VERSION3:
        strcpy(buffer, "<VERSION3>");
        break;

    case VERSION4:
        strcpy(buffer, "<VERSION4>");
        break;

    default:
        strcpy(buffer, "<BAD>");
        break;
    }

    return(buffer);
}


#undef SELF
#define SELF "decode_eopts"
char *decode_eopts(unsigned char extopt)
{
    extern char     str_buffer[];

    strcpy(str_buffer, "<"); 

    if (extopt == '\0')
    {
        strcpy(str_buffer, "<NONE>");

        return(str_buffer);
    }

    if (extopt & WAIT)
    {
        strcat(str_buffer, "WAIT|");
    }

    if (extopt & RESET)
    {
        strcat(str_buffer, "RESET|");
    }

    if (extopt & VIRTUAL)
    {
        strcat(str_buffer, "VIRTUAL|");
    }

    if (extopt & CONTINUOUS)
    {
        strcat(str_buffer, "CONTINUOUS|");
    }

    if (extopt & RANGE)
    {
        strcat(str_buffer, "RANGE|");
    }

    if (extopt & CLEAN_DRIVE)
    {
        strcat(str_buffer, "CLEAN_DRIVE|");
    }

    /* Check for bad value (none of the above) */
    if (strlen(str_buffer) == 1)
    {
        strcat(str_buffer, "BAD ");
    }

    /* terminate the string */
    str_buffer[strlen(str_buffer) - 1] = '>';

    return(str_buffer);
}


/*
 * Test the status in the final response header to see if there is a 
 * variable part to the response packet.
 */
#undef SELF
#define SELF "no_variable_part"
BOOLEAN no_variable_part()
{
    if ((status != STATUS_SUCCESS) && 
        (status != STATUS_DONE) && 
        (status != STATUS_RECOVERY_COMPLETE) && 
        (status != STATUS_NORMAL) && 
        (status != STATUS_VALID))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*
 * show the register status
 */
#undef SELF
#define SELF "st_show_register_status"
void st_show_register_status(STATUS status)
{
    switch (status)
    {
    case STATUS_INVALID_VERSION:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_INVALID_VERSION\n", SELF);
        break;

    case STATUS_MESSAGE_TOO_LARGE:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_MESSAGE_TOO_LARGE\n", 
                SELF);
        break;

    case STATUS_MESSAGE_TOO_SMALL:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_MESSAGE_TOO_SMALL\n", 
                SELF);
        break;

    case STATUS_MISSING_OPTION:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_MISSING_OPTION\n", SELF);
        break;

    case STATUS_PROCESS_FAILURE:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_PROCESS_FAILURE\n", SELF);
        break;

    case STATUS_INVALID_EVENT_CLASS:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_INVALID_EVENT_CLASS\n", 
                SELF);
        break;

    case STATUS_SUCCESS:
        CDR_MSG(0, "\t%s: REGISTER_STATUS = STATUS_SUCCESS\n", SELF);
        break;

    default:
        CDR_MSG(0, "\t%s: REGISTER_STATUS is invalid", SELF);
    }
}


/*
 * Name:
 *
 *        st_show_register_info()
 *
 * Description:
 *
 *        This function prints data from the server for the REGISTER
 *        requests made by main. 
 *        it does no checking.
 *
 * Explicit Inputs:
 *
 *        stat - pointer to the status of the panel.
 *        from_server - pointer to the panel info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_register_info"
void st_show_register_info(ACS_REGISTER_RESPONSE *from_server)
{
    char   type, event_class[30], vol_type[30], volid[7], resource_data_type;
    int    i, event_type=0;

    CDR_MSG(4, "\t%s: event_sequence is %d\n", SELF, from_server->event_sequence); 

    if ((from_server->event_reply_type == EVENT_REPLY_REGISTER) || 
        (from_server->event_reply_type == EVENT_REPLY_UNREGISTER) || 
        (from_server->event_reply_type == EVENT_REPLY_SUPERCEDED) || 
        (from_server->event_reply_type == EVENT_REPLY_SHUTDOWN))
    {
        CDR_MSG(4, "\t%s: registration_id is: %s\n", 
                SELF, 
                from_server->event.event_register_status.registration_id.registration);

        if (from_server->event_reply_type == EVENT_REPLY_REGISTER)
        {
            CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_REGISTER\n", SELF);
        }

        if (from_server->event_reply_type == EVENT_REPLY_UNREGISTER)
        {
            CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_UNREGISTER\n", SELF);
        }

        if (from_server->event_reply_type == EVENT_REPLY_SUPERCEDED)
        {
            CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_SUPERCEDED\n", SELF);
        }

        if (from_server->event_reply_type == EVENT_REPLY_SHUTDOWN)
        {
            CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_SHUTDOWN\n", SELF);
        }

        st_show_event_register_status(&from_server->event.event_register_status);
    }
    /* For an EVENT_VOLUME_STATUS  */
    else if (from_server->event_reply_type == EVENT_REPLY_VOLUME)
    {
        CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_VOLUME\n", SELF);
        event_type = from_server->event.event_volume_status.event_type;

        switch (event_type)
        {
        case VOL_ENTERED:
            strcpy(vol_type, "VOL_ENTERED");
            break;

        case VOL_ADDED:
            strcpy(vol_type, "VOL_ADDED");
            break;

        case VOL_REACTIVATED:
            strcpy(vol_type, "VOL_REACTIVATED");
            break;

        case VOL_EJECTED:
            strcpy(vol_type, "VOL_EJECTED");
            break;

        case VOL_DELETED:
            strcpy(vol_type, "VOL_DELETED");
            break;

        case VOL_MARKED_ABSENT:
            strcpy(vol_type, "VOL_MARKED_ABSENT");
            break;

        default:
            strcpy(vol_type, "none");
        }

        CDR_MSG(4, "\t%s: VOL_EVENT_TYPE is: %s\n", SELF, vol_type);
        memset(volid, 0, sizeof(volid));
        strcpy(volid, from_server->event.event_volume_status.vol_id.external_label);
        CDR_MSG(4, "\t%s: VOLID is: %s\n", SELF, volid);
    }
    /* For an EVENT_DRIVE_STATUS  */
    else if (from_server->event_reply_type == EVENT_REPLY_DRIVE_ACTIVITY)
    {
        st_show_event_drive_status(from_server);
    }
    /* For an EVENT_RESOURCE_STATUS  */
    else if (from_server->event_reply_type == EVENT_REPLY_RESOURCE)
    {
        st_show_event_resource_status(from_server);
    }
    else if (from_server->event_reply_type == EVENT_REPLY_CLIENT_CHECK)
    {
        CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_CLIENT_CHECK\n", SELF);
        CDR_MSG(4, "\t%s: responding with a check_registration commend\n", SELF);
        c_check_registration();
        CDR_MSG(4, "\t%s: completed call to check_registration\n", SELF);
    }
    /* Must have a problem because it's not a valid status */
    else
    {
        CDR_MSG(0, "\t%s: Not a valid resource status\n");
    }
}


/*
 * Name:
 *
 *        st_show_event_register_status()
 *
 * Description:
 *
 *        This function prints data from the server for the REGISTER
 *        requests made by main. 
 *        it does no checking.
 *
 * Explicit Inputs:
 *
 *        stat - pointer to the status of the panel.
 *        from_server - pointer to the panel info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_event_register_status"
void st_show_event_register_status(EVENT_REGISTER_STATUS *from_server)
{
    int             i, count = 0;
    char        event_class[40]; /* Bug before: used to be 20 */
    EVENT_CLASS_TYPE ev_class;
    EVENT_CLASS_REGISTER_RETURN register_return;

    count = from_server->count;
    CDR_MSG(4, "\t%s: COUNT is: %d\n", SELF, count);

    for (i = 0; i < count; i++)
    {
        ev_class = from_server->register_status[i].event_class;
        if (ev_class == EVENT_CLASS_VOLUME)
        {
            strcpy(event_class, "EVENT_CLASS_VOLUME");
        }
        else if (ev_class == EVENT_CLASS_RESOURCE)
        {
            strcpy(event_class, "EVENT_CLASS_RESOURCE");
        }
        else if (ev_class == EVENT_CLASS_DRIVE_ACTIVITY)
        {
            strcpy(event_class, "EVENT_CLASS_DRIVE_ACTIVITY");
        }

        CDR_MSG(4, "\t%s: EVENT_CLASS is: %s\n", SELF, event_class);
        register_return = from_server->register_status[i].register_return;

        if (register_return == EVENT_REGISTER_REGISTERED)
        {
            CDR_MSG(4, "\t%s: REGISTER_RETURN is: EVENT_REGISTER_REGISTERED\n",SELF);
        }
        else if (register_return == EVENT_REGISTER_UNREGISTERED)
        {
            CDR_MSG(4, "\t%s: REGISTER_RETURN is: EVENT_REGISTER_UNREGISTERED\n", SELF);
        }
        else if (register_return == EVENT_REGISTER_INVALID_CLASS)
        {
            CDR_MSG(4, "\t%s: REGISTER_RETURN is: EVENT_REGISTER_INVALID_CLASS\n", SELF);
        }
    }
}


/*
 * Name:
 *
 *        st_show_event_resource_status()
 *
 * Description:
 *
 *        This function prints data from the server for the REGISTER
 *        requests made by main. 
 *        it does no checking.
 *
 * Explicit Inputs:
 *
 *        stat - pointer to the status of the panel.
 *        from_server - pointer to the panel info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_event_resourse_status"
void st_show_event_resource_status(ACS_REGISTER_RESPONSE *from_server)
{
    char fsc[4], serial_nbr;
    signed char sense_key, asc, ascq;
    RESOURCE_DATA_TYPE resource_data_tp;

    CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_RESOURCE\n", SELF);
    CDR_MSG(4, "\t%s: RESOURCE_TYPE is: %s\n", 
            SELF, cl_type(from_server->event.event_resource_status.resource_type));

    CDR_MSG(4, "\t%s: RESOURCE_IDENTIFIER is: %s\n", 
            SELF, 
            cl_identifier(from_server->event.event_resource_status.resource_type,
                          &from_server->event.event_resource_status.resource_identifier));
    CDR_MSG(4, "\t%s: RESOURCE_EVENT is: %s\n", 
            SELF, 
            cl_resource_event(from_server->event.
                              event_resource_status.resource_event));

    resource_data_tp = from_server->event.event_resource_status.
                       resource_data_type;

    if (resource_data_tp == SENSE_TYPE_NONE)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is: SENSE_TYPE_NONE\n", SELF);

        return;
    }
    else if (resource_data_tp == SENSE_TYPE_HLI)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is: SENSE_TYPE_HLI\n", SELF);
        CDR_MSG(4, "\t%s: SENSE_HLI category: %d, code: %d\n", 
                SELF, 
                from_server->event.event_resource_status.resource_data.sense_hli.
                category, 
                from_server->event.event_resource_status.
                resource_data.sense_hli.code);
    }
    else if (resource_data_tp == SENSE_TYPE_SCSI)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is: SENSE_TYPE_SCSI\n", SELF);
        CDR_MSG(4, "\t%s: SENSE_SCSI sense_key is: %02X asc is: %02X ascq is: %02X\n", 
                SELF, 
                from_server->event.event_resource_status.
                resource_data.sense_scsi.sense_key,
                from_server->event.event_resource_status.
                resource_data.sense_scsi.asc,
                from_server->event.event_resource_status.
                resource_data.sense_scsi.ascq);
    }
    else if (resource_data_tp == SENSE_TYPE_FSC)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is: SENSE_TYPE_FSC\n", SELF);
        memset(fsc, 0, sizeof(fsc));
        strcpy(fsc, from_server->event.event_resource_status.
               resource_data.sense_fsc.fsc);
        CDR_MSG(4, "\t%s: SENSE_FSC is: %s \n", SELF, fsc);
    }
    else if (resource_data_tp == RESOURCE_CHANGE_SERIAL_NUM)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is : RESOURCE_CHANGE_SERIAL_NUM\n", SELF);
        CDR_MSG(4, "\t%s: SERIAL_NUMBER is: %s\n", 
                SELF, 
                &from_server->event.event_resource_status.
                resource_data.serial_num.serial_nbr[0]);
    }
    else if (resource_data_tp == RESOURCE_CHANGE_LSM_TYPE)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is: RESOURCE_CHANGE_LSM_TYPE\n",SELF);
        CDR_MSG(4, "\t%s: LSM type is: %d\n", 
                SELF, 
                from_server->event.event_resource_status.resource_data.
                lsm_type);
    }
    else if (resource_data_tp == RESOURCE_CHANGE_DRIVE_TYPE)
    {
        CDR_MSG(4, "\t%s: RESOURCE_DATA_TYPE is : RESOURCE_CHANGE_DRIVE_TYPE\n",SELF);
        CDR_MSG(4, "\t%s: DRIVE_TYPE  is: %d\n", 
                SELF, 
                from_server->event.event_resource_status.resource_data.drive_type);
    }
    /* no Resource data type .....should never get here */
    else
    {
        CDR_MSG(0, "\t%s: Bad RESOURCE_DATA_TYPE!\n");
    }
}


/*
 * Name:
 *
 *        st_show_display_info()
 *
 * Description:
 *
 *        This function prints data from the server for the DISPLAY
 *        requests made by main. 
 *        it does no checking.
 *
 *
 *
 */
#undef SELF
#define SELF "st_show_display_info"
void st_show_display_info(ACS_DISPLAY_RESPONSE *from_server)
{
    char xml_buf[4096];
    memset(xml_buf, 0, sizeof(xml_buf));

    CDR_MSG(4, "\t%s DISPLAY TYPE is: %s\n", 
            SELF, acs_type(from_server->display_type));
    CDR_MSG(4, "\t%s DISPLAY XML_DATA_LENGTH is: %d\n", 
            SELF, from_server->display_xml_data.length);

    if (from_server->display_xml_data.length > 0)
    {
        strncpy(xml_buf, 
                from_server->display_xml_data.xml_data, 
                from_server->display_xml_data.length);

        CDR_MSG(4, "\t%s DISPLAY xml_data is: %s\n", SELF, xml_buf);
    }
}


/*
 * Name:
 *
 *        st_show_event_drive_status()
 *
 * Description:
 *
 *        This function prints data from the server for the REGISTER
 *        requests made by main.
 *        it does no checking.
 *
 * Explicit Inputs:
 *
 *        from_server - pointer to the drive activity info structure.
 *
 * Return Values:
 *
 *        None.
 */
#undef SELF
#define SELF "st_show_event_drive_status"
void st_show_event_drive_status(ACS_REGISTER_RESPONSE *from_server)
{
    static char *volume_type_str[13] = 
    { 
        "VOLUME_TYPE_FIRST",
        "VOLUME_TYPE_DIAGNOSTIC",
        "VOLUME_TYPE_STANDARD",
        "VOLUME_TYPE_DATA",
        "VOLUME_TYPE_SCRATCH",
        "VOLUME_TYPE_CLEAN",
        "VOLUME_TYPE_MVC",
        "VOLUME_TYPE_VTV",
        "VOLUME_TYPE_SPENT_CLEANER",
        "VOLUME_TYPE_MEDIA_ERROR",
        "VOLUME_TYPE_UNSUPPORTED_MEDIA",
        "VOLUME_TYPE_C_OR_D",
        "VOLUME_TYPE_LAST"
    };

    time_t        activity_time;
    RESOURCE_DATA res_data;

    CDR_MSG(4, "\t%s: EVENT_REPLY_TYPE is: EVENT_REPLY_DRIVE_ACTIVITY\n", SELF);
    CDR_MSG(4, "\t%s: DRIVE ACTIVITY TYPE is : %s\n", 
            SELF, cl_type(from_server->event.event_drive_status.event_type));

    res_data = from_server->event.event_drive_status.resource_data;

    if (from_server->event.event_drive_status.event_type == TYPE_MOUNT)
    {
        activity_time = (time_t) res_data.drive_activity_data.start_time;
        CDR_MSG(4, "\t%s: MOUNT START TIME is : ", SELF);
        CDR_MSG(4, ctime(&activity_time));

        activity_time = (time_t) res_data.drive_activity_data.completion_time;
        CDR_MSG(4, "\t%s: MOUNT COMPLETION TIME is : ", SELF);
        CDR_MSG(4, ctime(&activity_time));
    }
    else
    {
        activity_time = (time_t) res_data.drive_activity_data.start_time;
        CDR_MSG(4, "\t%s: DISMOUNT START TIME is : ", SELF);
        CDR_MSG(4, ctime(&activity_time));

        activity_time = (time_t) res_data.drive_activity_data.completion_time;
        CDR_MSG(4, "\t%s: DISMOUNT COMPLETION TIME is : ", SELF);
        CDR_MSG(4, ctime(&activity_time));
    }

    CDR_MSG(4, "\t%s: VOLUME ID is : %s\n",
            SELF, res_data.drive_activity_data.vol_id.external_label);
    CDR_MSG(4, "\t%s: VOLUME TYPE is : %s\n",
            SELF, volume_type_str[res_data.drive_activity_data.volume_type]);
    CDR_MSG(4, "\t%s: DRIVE ID is : %d, %d, %d, %d\n", 
            SELF,
            res_data.drive_activity_data.drive_id.panel_id.lsm_id.acs,
            res_data.drive_activity_data.drive_id.panel_id.lsm_id.lsm,
            res_data.drive_activity_data.drive_id.panel_id.panel,
            res_data.drive_activity_data.drive_id.drive);
    CDR_MSG(4, "\t%s: POOL ID is : %d\n", 
            SELF, res_data.drive_activity_data.pool_id.pool);
    CDR_MSG(4, "\t%s: HOME LOCATION is : %d, %d, %d, %d, %d\n", 
            SELF,
            res_data.drive_activity_data.home_location.panel_id.lsm_id.acs,
            res_data.drive_activity_data.home_location.panel_id.lsm_id.lsm,
            res_data.drive_activity_data.home_location.panel_id.panel,
            res_data.drive_activity_data.home_location.row,
            res_data.drive_activity_data.home_location.col);
}


void st_output(int   messageLevel,
               char *functionName,
               int   lineNumber,
               char *storageAddress,
               int   storageLength)
{
#define DEFAULT_MSGLVL  4
    char                cdrMsglvlVar[9];
    int                 i;
    int                 thresholdMsglvl = DEFAULT_MSGLVL;   

    /*****************************************************************/
    /* If CDR_MSGLVL is undefined, then set threshold level to       */
    /* default.  Otherwise, set threshold level to specified         */
    /* CDR_MSGLVL.                                                   */
    /*                                                               */
    /* Any messages that specify a messageLevel <= threshold level   */
    /* are output to both STDOUT and trace.file.  Otherwise, they    */
    /* are output to the trace file only.                            */
    /*****************************************************************/
    if (getenv(CDR_MSGLVL_VAR) != NULL)
    {
        memset(cdrMsglvlVar, 0 , sizeof(cdrMsglvlVar));
        strncpy(cdrMsglvlVar, getenv(CDR_MSGLVL_VAR), (sizeof(cdrMsglvlVar) - 1));
        thresholdMsglvl = atoi(cdrMsglvlVar);
    }

    if (messageLevel <= thresholdMsglvl)
    {
        printf(st_output_buffer);
    }

    /*****************************************************************/
    /* When writing output to trace, convert any leading '\t' or     */
    /* '\n' to spaces.                                               */
    /*****************************************************************/
    for (i = 0;
        i < strlen(st_output_buffer);
        i++)
    {
        if ((st_output_buffer[i] != '\n') &&
            (st_output_buffer[i] != '\t'))
        {
            break;
        }
        else
        {
            st_output_buffer[i] = ' ';
        }
    }

    srvtrcc(__FILE__, 
            functionName, 
            lineNumber, 
            TRCI_CDKTYPE, 
            storageAddress, 
            storageLength, 
            st_output_buffer); 

    return;
}






