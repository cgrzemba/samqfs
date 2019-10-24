/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qmedia.c                                    */
/** Description:    XAPI QUERY MIXED MEDIA INFO processor.           */
/**                                                                  */
/**                 Return ACSAPI drive type and media type          */
/**                 information.                                     */
/**                                                                  */
/**                 NOTE: Currently, there is no XAPI                */
/**                 QUERY MIXED MEDIA INFO transaction that is sent  */
/**                 to the XAPI server (however there are            */
/**                 separate XAPI <query_drvtypes> and XAPI          */
/**                 <query_media> transactions that are processed    */
/**                 when the XAPI client is initialized).            */
/**                                                                  */
/**                 QUERY MIXED MEDIA INFO is processed              */
/**                 locally by returning ACSAPI drive type and       */
/**                 media type information from the XAPI client      */
/**                 XAPIDRVTYP and XAPIMEDIA tables.                 */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/29/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <stdlib.h>
#include <stddef.h>                  
#include <stdio.h>
#include <string.h>

#include "api/defs_api.h"                              
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qmedia                                       */
/** Description:   XAPI QUERY MIXED MEDIA INFO processor.            */
/**                                                                  */
/** Return all of the ACSAPI drive type and media type information   */
/** stored in the XAPI client XAPIDRVTYPE and XAPIMEDIA tables.      */
/**                                                                  */
/** The XAPIDRVTYP table is a conversion table to convert XAPI       */
/** model names into ACSAPI drive type codes (and vice versa).       */
/** In addition the XAPIDRVTYP table entry for a single ACSAPI       */
/** drive type code contains a table of compatible ACSAPI            */
/** media type codes (see the XAPIMEDIA table).                      */
/**                                                                  */
/** The XAPIMEDIA table is a conversion table to convert XAPI        */
/** media names into ACSAPI media type codes (and vice versa).       */
/** In addition the XAPIMEDIA table entry for a single ACSAPI        */
/** media type code contains a table of compatible ACSAPI            */
/** drive type codes (see the XAPIDRVTYP table).                     */
/**                                                                  */
/** The XAPIDRVTYPE and XAPIMEDIA tables are populated by xapi_main  */
/** when the XAPI client is initialized.  Together, the XAPIDRVTYP   */
/** and XAPIMEDIA tables are the equivalent of the MVS SRMM tables.  */
/**                                                                  */
/** The QUERY MIXED MEDIA INFO command is allowed to proceed even    */
/** when the XAPI client is in the IDLE state.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MIXED MEDIA INFO request consists of:           */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MIXED MEDIA INFO response consists of:          */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_RESPONSE data consisting of:                          */
/**      i.   type                                                   */
/**      ii.  QU_MMI_RESPONSE data consisting of:                    */
/**           a.   media_type_count                                  */
/**           b.   QU_MEDIA_TYPE_STATUS[media_type_count] data       */
/**                entries consisting of:                            */
/**                1.   media_type                                   */
/**                2.   media_type_name (10 characters)              */
/**                3.   cleaning_cartridge which is either:          */
/**                     i.   CLN_CART_NEVER                          */
/**                     ii.  CLN_CART_INDETERMINATE                  */
/**                     iii. CLN_CART_ALWAYS                         */
/**                4.   max_cleaning_useage                          */
/**                5.   compat_count                                 */
/**                6.   compat_drive_types[MM_MAX_COMPAT_TYPES]      */
/**           c.   drive_type_count                                  */
/**           d.   QU_DRIVE_TYPE_STATUS[drive_type_count] data       */
/**                entries consisting of:                            */
/**                1.   drive_type                                   */
/**                2.   drive_type_name (10 characters)              */
/**                3.   compat_count                                 */
/**                4.   compat_media_types[MM_MAX_COMPAT_TYPES]      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qmedia"

extern int xapi_qmedia(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 qmmiResponseSize;
    int                 i;
    int                 j; 
    int                 drvRemainingCount;
    int                 drvResponseCount;
    int                 drvIndex;
    int                 medRemainingCount;
    int                 medResponseCount;
    int                 medIndex;

    struct XAPIDRVTYP  *pXapidrvtyp;
    struct XAPIMEDIA   *pXapimedia;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_MMI_RESPONSE    *pQu_Mmi_Response    = &(pQuery_Response->status_response.mm_info_response);
    QU_MEDIA_TYPE_STATUS *pQu_Media_Type_Status = &(pQu_Mmi_Response->media_type_status[0]);
    QU_DRIVE_TYPE_STATUS *pQu_Drive_Type_Status = &(pQu_Mmi_Response->drive_type_status[0]);

    qmmiResponseSize = (char*) pQu_Mmi_Response -
                       (char*) pQuery_Response +
                       (sizeof(QU_MMI_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY MMI request=%08X, size=%d, "
           "QUERY MMI response=%08X, qmmiResponseSize=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pQuery_Response,
           qmmiResponseSize);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         qmmiResponseSize);

    if ((pXapicvt->xapidrvtypCount == 0) ||
        (pXapicvt->xapimediaCount == 0))
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          qmmiResponseSize,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Build the ACSAPI responses.                                   */
    /*===============================================================*/
    /* Each response will contain information about either           */
    /* drive_type codes and/or media_type codes.                     */
    /*****************************************************************/
    pXapimedia = &(pXapicvt->xapimedia[0]);
    medRemainingCount = pXapicvt->xapimediaCount;
    pXapidrvtyp = &(pXapicvt->xapidrvtyp[0]);
    drvRemainingCount = pXapicvt->xapidrvtypCount;

    while (1)
    {
        qmmiResponseSize = (char*) pQu_Mmi_Response -
                           (char*) pQuery_Response +
                           (sizeof(QU_MMI_RESPONSE));

        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qmmiResponseSize);

        if (medRemainingCount > MAX_ID)
        {
            medResponseCount = MAX_ID;
        }
        else
        {
            medResponseCount = medRemainingCount;
        }

        if (drvRemainingCount > MAX_ID)
        {
            drvResponseCount = MAX_ID;
        }
        else
        {
            drvResponseCount = drvRemainingCount;
        }

        medIndex = pXapicvt->xapimediaCount - medRemainingCount;
        drvIndex = pXapicvt->xapidrvtypCount - drvRemainingCount;

        pQu_Media_Type_Status = &(pQu_Mmi_Response->media_type_status[0]);
        pQu_Drive_Type_Status = &(pQu_Mmi_Response->drive_type_status[0]);

        TRMSGI(TRCI_XAPI,
               "At top of while; medRemaining=%d, medIndex=%d, medResponse=%d, "
               "drvRemaining=%d, drvIndex=%d, drvResponse=%d, MAX_ID=%d\n",
               medRemainingCount,
               medIndex,
               medResponseCount,
               drvRemainingCount,
               drvIndex,
               drvResponseCount,
               MAX_ID);

        if (medRemainingCount > 0)
        {
            pQu_Mmi_Response->media_type_count = medRemainingCount;

            for (i = medIndex;
                i < (medIndex + medResponseCount);
                i++, pXapimedia++, pQu_Media_Type_Status++)
            {
                pQu_Media_Type_Status->media_type = 
                (MEDIA_TYPE) pXapimedia->acsapiMediaType;

                strcpy(pQu_Media_Type_Status->media_type_name, 
                       pXapimedia->mediaNameString);

                pQu_Media_Type_Status->cleaning_cartridge = 
                pXapimedia->cleanerMediaFlag;

                pQu_Media_Type_Status->compat_count = 
                pXapimedia->numCompatDrives;

                for (j = 0;
                    j < pXapimedia->numCompatDrives;
                    j++)
                {
                    pQu_Media_Type_Status->compat_drive_types[j] = 
                    pXapimedia->compatDriveType[j];
                }
            }
        }

        if (drvRemainingCount > 0)
        {
            pQu_Mmi_Response->drive_type_count = drvRemainingCount;

            for (i = drvIndex;
                i < (drvIndex + drvResponseCount);
                i++, pXapidrvtyp++, pQu_Drive_Type_Status++)
            {
                pQu_Drive_Type_Status->drive_type = 
                (DRIVE_TYPE) pXapidrvtyp->acsapiDriveType;

                strcpy(pQu_Drive_Type_Status->drive_type_name, 
                       pXapidrvtyp->modelNameString);

                pQu_Drive_Type_Status->compat_count = 
                pXapidrvtyp->numCompatMedias;

                for (j = 0;
                    j < pXapidrvtyp->numCompatMedias;
                    j++)
                {
                    pQu_Drive_Type_Status->compat_media_types[j] = 
                    pXapidrvtyp->compatMediaType[j];
                }
            }
        }

        if (drvResponseCount > 0)
        {
            qmmiResponseSize = (char*) &(pQu_Mmi_Response->drive_type_status[0]) -
                               (char*) pQuery_Response +
                               ((sizeof(QU_DRIVE_TYPE_STATUS)) * drvResponseCount);
        }
        else
        {
            qmmiResponseSize = (char*) &(pQu_Mmi_Response->drive_type_status[0]) -
                               (char*) pQuery_Response +
                               ((sizeof(QU_DRIVE_TYPE_STATUS)) * 1);
        }

        TRMSGI(TRCI_XAPI,
               "offsetof(QU_DRIVE_TYPE_STATUS)=%d, "
               "sizeof(QU_DRIVE_TYPE_STATUS)=%d, "
               "drvReponseCount=%d, qmmiResponseSize=%d\n",
               ((char*) &(pQu_Mmi_Response->drive_type_status[0]) - (char*) pQuery_Response),
               sizeof(QU_DRIVE_TYPE_STATUS),
               drvResponseCount,
               qmmiResponseSize);

        medRemainingCount -= medResponseCount;
        drvRemainingCount -= drvResponseCount;

        if ((medRemainingCount > 0) ||
            (drvRemainingCount > 0))
        {
            xapi_int_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmmiResponseSize);
        }
        else
        {
            xapi_fin_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmmiResponseSize);

            break;
        }
    }

    return queryRC;
}



