/* SccsId @(#)csi_header.h	1.2 1/11/94  */
#ifndef _CSI_HEADER_
#define _CSI_HEADER_

/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Header file containing all CSI_HEADER structure-specific definitions.
 *
 * Modified by:
 *
 *      J. W. Wishner   30-Jan-1989.    Original.
 *	J. A. Wishner   05/01/89.	TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *      E. A. Alongi    03-Dec-1992.    Changes resulting from flexelint run.
 *      E. A. Alongi    03-Dec-1993.    Removed the adi specific and rpc 
 *			specific declarations and placed them in 
 *			csi_adi_header.h and csi_rpc_header.h respectively. The
 *			common declarations were left here.
 */

/*
 *      Header Files:
 *		Some of the header files were moved below because they depend
 *		upon structure definitions CSI_SYNTAX, CSI_PROTOCOL and
 *		CSI_CONNECT.  The rpc/rpc.h include needs to be here since
 *		the ADI code uses csi.h which was not $ifdef'ed divided 
 *		properly.
 */

#include <rpc/rpc.h>

/*
 *	Defines, Typedefs and Structure Definitions:
 */

/*
 *      Procedure Type Declarations:
 */

/*
 *	Common declarations:
 */

typedef enum {
    CSI_SYNTAX_NONE		= 0,	/* default transfer syntax is none */
    CSI_SYNTAX_XDR 			/* XDR used as transfer syntax */
} CSI_SYNTAX;

typedef enum {
    CSI_PROTOCOL_TCP		= 1,	/* transport protocol used is TCP/IP */
    CSI_PROTOCOL_UDP		= 2,	/* transport protocol used is TCP/IP */
    CSI_PROTOCOL_ADI		= 3 	/* transport protocol used is OSLAN  */
} CSI_PROTOCOL;

typedef enum {
    CSI_CONNECT_RPCSOCK		= 1,	/* type of connection defined by CSI */
    CSI_CONNECT_ADI		= 2 
} CSI_CONNECT; 

/*
 *      Header Files:
 */

#ifndef ADI /* the RPC include and defines */

#ifndef _CSI_RPC_HEADER_
#include "csi_rpc_header.h"
#endif

#define CSI_HEADER 	CSI_HEADER_RPC
#define CSI_XID		CSI_XID_RPC

#else /* the ADI include and defines */

#ifndef _CSI_ADI_HEADER_
#include "csi_adi_header.h"
#endif

#define CSI_HEADER 	CSI_HEADER_ADI
#define CSI_XID		CSI_XID_ADI

#endif /* ~ADI */

#endif /* _CSI_HEADER_ */

