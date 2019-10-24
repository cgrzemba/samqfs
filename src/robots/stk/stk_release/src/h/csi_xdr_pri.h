/*  SccsId     @(#)csi_xdr_pri.h	5.3 10/12/94  */
#ifndef _CSI_XDR_
#define _CSI_XDR_
/*
 *
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 * Name:        
 *      csi_xdr.h
 *
 * Functional Description:      
 *
 *      CSI/SSI interface include file for the client system interface.
 *
 * Modified by:
 *      
 *	E. A. Alongi	20-Sep-1993.	Original.
 *	E. A. Alongi	29-Sep-1993.	Added xdrmem_create(), deleted all
 *					extern statements, and included xdr.h.
 *	D. A. Myers	12-Oct-1994	Porting changes
 */

#ifndef _rpc_xdr_h
#include <rpc/xdr.h>
#endif

void 	xdrmem_create(XDR *, char *, u_int, enum xdr_op);
bool_t	xdr_void (void);
bool_t	xdr_int (XDR *, int *);
bool_t	xdr_u_int (XDR *, u_int *);
bool_t	xdr_long (XDR *, long *);
bool_t	xdr_u_long (XDR *, u_long *);
bool_t	xdr_short (XDR *, short *);
bool_t	xdr_u_short (XDR *, u_short *);
bool_t	xdr_bool (XDR *, bool_t *);
bool_t	xdr_enum (XDR *, enum_t *);
bool_t	xdr_array (XDR *, char **, u_int *, u_int, u_int, xdrproc_t);
bool_t	xdr_bytes (XDR *, char **, u_int *, u_int);
bool_t	xdr_opaque (XDR *, caddr_t, u_int);
bool_t	xdr_string (XDR *, char **, u_int);
bool_t	xdr_char (XDR *, char *);
bool_t	xdr_wrapstring (XDR *, char **);
bool_t	xdr_reference (XDR *, caddr_t *, u_int, xdrproc_t);
bool_t	xdr_pointer (XDR *, char **, u_int, xdrproc_t);
bool_t	xdr_u_char (XDR *, u_char *);
bool_t	xdr_vector (XDR *, char *, u_int, u_int, xdrproc_t);
bool_t	xdr_float (XDR *, float *);
bool_t	xdr_double (XDR *, double *);
void	xdr_free (xdrproc_t, char *);
#endif /*_CSI_XDR_*/
