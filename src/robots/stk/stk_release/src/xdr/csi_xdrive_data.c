# ifndef lint
static char SccsId[] = "@(#)csi_xdrive_data.c   7.1 06/03/04 ";
# endif
/*
 * Copyright (2004, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xdrive_data()
 *
 * Description:
 *
 *      Routine serializes/deserializes a DRIVE_ACTIVITY_DATA struct.
 *
 * Return Values:
 *
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History
 *      Wipro(Subhash)     03-Jun-2004     Created.
 */
/*
 *      Header Files:
 */

# include "csi.h"
# include "ml_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char *st_src = __FILE__;
static char *st_module = "csi_xdrive_data()";
/*
 *      Procedure Type Declarations:
 */
bool_t
csi_xdrive_data(
        XDR                 *xdrsp,          /* xdr handle structure */
        DRIVE_ACTIVITY_DATA *drive_data      /* drive activity data ptr */
)
{
# ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,     /* routine name */
             2,         /* parameter count */
             (unsigned long) xdrsp, /* argument list */
             (unsigned long) drive_data);
                        /* argument list */
# endif /* DEBUG */

    /* translate the start time member of drive activity data */
    if (!xdr_long(xdrsp, &drive_data->start_time))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "xdr_long()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the completion time member of drive activity data */
    if (!xdr_long(xdrsp, &drive_data->completion_time))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "xdr_long()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the volume id member of drive activity data */
    if (!csi_xvol_id(xdrsp, &drive_data->vol_id))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "csi_xvol_id()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the volume type member of drive activity data */
    if (!xdr_enum(xdrsp, (enum_t *) &drive_data->volume_type))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "xdr_enum()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the drive id member of drive activity data */
    if (!csi_xdrive_id(xdrsp, &drive_data->drive_id))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "csi_xdrive_id()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the pool id member of drive activity data */
    if (!csi_xpool_id(xdrsp, &drive_data->pool_id))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "csi_xpool_id()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }

    /* translate the home location member of drive activity data */
    if (!csi_xcell_id(xdrsp, &drive_data->home_location))
    {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
             "csi_xcell_id()",
             MMSG(928, "XDR message translation failure")));
        return (0);
    }
    return (1);
}

