/* SccsId @(#)csi_xdr_xlate.h	1.2 1/11/94  */
#ifndef _CSI_XDR_XLATE_
#define _CSI_XDR_XLATE_              /* where MODULE == header module name */
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file contains definition particular to the csi translation
 *      functions csi_xdrreque.c csi_xdrrespo.c.
 *
 * Modified by:
 *
 *      J. A. Wishner   12-Apr-1989.    Created.
 *      J. A. Wishner   05/01/89.       TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *      J. A. Wishner   09/26/90.       cur_size and exp_size renamed and made
 *                                      global: csi_xcur_size,csi_xexp_size.
 *
 */

/*
 *      Header Files:
 */

/*
 *      Defines, Typedefs and Structure Definitions:
 *
 *
 *      Considerations:
 *
 */
/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define CHECKSIZE(cur_size, obj_size, tot_size)                               \
    (cur_size + obj_size > tot_size) ?  TRUE : FALSE
 
#define RETURN_PARTIAL_PACKET(xdrsp, bufferp)                                 \
{                                                                             \
  if (XDR_DECODE == xdrsp->x_op) {                                            \
      bufferp->size = csi_xcur_size;                                          \
      bufferp->translated_size = csi_xcur_size;                               \
      bufferp->packet_status = (CSI_PAKSTAT_INITIAL == bufferp->packet_status)\
              ? CSI_PAKSTAT_XLATE_ERROR : bufferp->packet_status;             \
      if (xdr_allocated)                                                      \
          bufferp->maxsize = csi_xcur_size;                                   \
  }                                                                           \
  else if (XDR_ENCODE == xdrsp->x_op) {                                       \
      bufferp->translated_size = csi_xcur_size;                               \
  }                                                                           \
  return(1);                                                                  \
}

#define RETURN_COMPLETE_PACKET(xdrsp, bufferp)                                \
{                                                                             \
                        bufferp->packet_status = CSI_PAKSTAT_XLATE_COMPLETED; \
                        RETURN_PARTIAL_PACKET(xdrsp, bufferp)                 \
}



/*
 *      Procedure Type Declarations:
 */
                                        /* external procedure declarations */
#endif /* _CSI_XDR_XLATE_ */




