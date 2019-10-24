#
# Copyright (C) 1989,2012, Oracle and/or its affiliates. All rights reserved.
# 
# Module:
# 
#      SSI makefile.   
# 
# Environment: 
# 
#      Tested for Sun SOLARIS, IBM AIX, and RedHat Linux 8.0.
#      Can be made to run on other releases of UNIX and non-UNIX
#      OS's by STK partners.
# 
# Modified by:
# 
#      J. A. Wishner       23-Jan-1989     Created..
#      R. P. Cushman       08-May-1990     Added lines to compile pd mod
#      J. S. Alexander     29-May-1990     Added STATS to CFLAGS.
#                                          Remd ACSSS=/we/joseph/acsss def.
#      J. W. Montgomery    18-Jun-1990     Version 2.
#      J. A. Wishner       23-Sep-1990     Modified to split out xdr funcs.
#      Howard I. Grapek    29-Aug-1991     Mods needed for R3 (VERSION2)
#      Howard L. Freeman   15-Sep-1991     Mods for R3 ACSAPI/CSI Test tape
#      J. A. Wishner       21-Oct-1991     Completed mods for R-3.
#      A. W. Steere        08-Nov-1991     Added test source files
#      D. A. Beidle        03-Dec-1991.    Added csi_chk_input module as a
#                                          substitute for the cl_chk_input module.
#      A. W. Steere        30-Jan-1992     moved .csrc .sb from clobber to
#                                          clean.
#      Howard I. Grapek    15-Oct-1992     Cleanup for toolkit.
#                                          Removed SQL, sb, lint refs, calls to CPR. 
#                                          Also removed t_* and placed in separate 
#                                          directory in tree. 
#      Ken Stickney        02-Dec-1993     Transformed into simpler product makefile. 
#                                          Took out development options/functions.
#      Howard I. Grapek    04-Jan-1994     Mods made for ANSI C.
#      H. I. Grapek        07-Jan-1994     Common_lib is libcl.a
#                                          Removed libdv, removed BIN variable.  
#                                          Miscellaneous cleanup for EC
#                                          Added inclusion of make.ini
#                                          Removed DEBUG and NONDEBUG variables 
#                                          (they are in make.ini)
#      H. I. Grapek        14-Jan-1994     Documentation, cleanup.
#      K. Stickney         23-Dec-1994     Library flag changes for
#                                          Solaris.
#      Mitch Black         25-Aug-2003     Added LIBRUNPATH for runtime path of 
#                                          shared object libraries (for Solaris).
#      Mitch Black         01-Dec-2004     Cleaned up - see diffs in Continuus.
#      Mike Williams       27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                          enable the ability to change from 
#                                          64-bit to 32-bit compile flags.
#                                          COMMON_CC_FLAGS is set in make.ini.
#                                          Updated the copyright statement.
#                                          Added CFLAGS to CC statement.
#      Joseph Nofi         15-May-2011     XAPI support;
#                                          Added -DXAPI compile option.
#                                          Added APILIB and XAPILIB to libraries.
#                                          Added srvcommon.h to header files.
#                                          Added LIBPTHREADS to compiler libraries.
#      Joseph Nofi         11-Sep-2012     Add variable XAPI support using
#                                          CDK_BUILD_OPTION variable.
#

#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

#	Defines

H		=../h
INSTALL		=../../bin

#	Flags/Options

CC              = $(COMPILER)
ADIFLAGS        = $(ADI_FLAGS)

DEFINE		=-DSSI
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) $(DEFINE) -D$(CDK_BUILD_OPTION) $(ADIFLAGS)

#	Libraries

LIB_LIST_BASE		= -z ignore -B dynamic -L $(OBJ_DIR) -lcl -lapi -lipc -lutl
LIB_LIST_XAPI		=../../lib/libxapi.a ../../lib/libcl.a

XDRLIB 		= $(OBJ_DIR)/libssixdr.so
APILIB		= $(OBJ_DIR)/libapi.so
OTHER_LIBS      = $(LIBANSI)
ADILIBS  	= $(ADI_LIBS)

HDR=	$(H)/csi.h \
	$(H)/csi_structs.h \
	$(H)/csi_v0_structs.h \
	$(H)/csi_v1_structs.h \
	$(H)/srvcommon.h 

SRC=    csi_main.c     \
	csi_chk_inpu.c \
	csi_fmtlmq_l.c \
	csi_fmtniq_l.c \
	csi_freeqmem.c \
	csi_getmsg.c   \
	csi_getiaddr.c \
	csi_globals.c  \
	csi_hostaddr.c \
	csi_ignoresa.c \
	csi_init.c     \
	csi_ipcdisp.c  \
	csi_ipc_send.c \
	csi_logevent.c \
	csi_net_send.c \
	csi_netbufin.c \
	csi_process.c  \
	csi_ptrace.c   \
	csi_qclean.c   \
	csi_qcmp.c     \
	csi_qinit.c    \
	csi_qget.c     \
	csi_qput.c     \
	csi_rpcinput.c \
	csi_rpctrans.c \
	csi_shutdown.c \
	csi_sighdlr.c  \
	csi_ssicmp.c   \
	csi_ssi_api.c  \
	csi_svcinit.c  \
	csi_rpccall.c  \
	csi_rpcdisp.c  \
	csi_rpctinit.c \
	csi_rpcuinit.c \
	csi_v0_response.c \
	csi_v1_response.c \
	csi_v2_response.c \
	csi_v4_response.c

OBJ  		= $(SRC:.c=.o)

ADISRC = $(ADI_FILE_LIST)

ADIOBJ = $(ADISRC:.c=.o)

EXEC	= ssi

#
# Targets
#

all:    	nondebug install 

$(EXEC):	$(OBJ) $(ADIOBJ) $(XDRLIB) $(APILIB)
	$(CC) -o $@ $(OBJ) $(ADIOBJ) $(CFLAGS) $(LIBRUNPATH) $(XDRLIB) $(LIB_LIST_$(CDK_BUILD_OPTION)) \
        $(APILIB) $(UCBLIB_DIR) $(LIBRPCSOC) $(LIB_DIR) $(LIBSOCKET) $(LIBNSL) $(LIBPTHREADS)
top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) $(ANSI_INCLUDE)"

install: 
	cp $(EXEC) $(INSTALL)

clean:
	rm -f $(OBJ) $(EXEC) 

sterile:
	rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#  Dependencies
#
