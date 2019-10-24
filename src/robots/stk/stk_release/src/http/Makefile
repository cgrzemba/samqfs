# SccsId - @(#)Makefile  
#                                                               
# Copyright (c) 2011, 2011, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      Makefile                                      
#                                                               
# Description:    t_http test server executable makefile                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720/CDK240  Joseph Nofi     08/15/11                       
#     Created for CDK to add XAPI support.
#                                                               
#===============================================================
#
# Include the compiler definitions needed to build on your specific host.
#
include ../../etc/make.ini

#	Defines

H		=../h
INSTALL		=../../bin

#	Flags/Options

CC              = $(COMPILER)

DEFINE		=-DXAPI
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) $(DEFINE) 

#	Libraries

APILIB		=../../lib/libapi.a
CLLIBS		=../../lib/libcl.a
XAPILIB		=../../lib/libxapi.a
OTHER_LIBS      = $(LIBANSI)

SRC=    http_main.c    \
	http_dismount.c  \
	http_eject.c  \
	http_enter.c  \
	http_gen_resp.c  \
	http_listen.c  \
	http_mount.c  \
	http_qacs.c  \
	http_qcap.c  \
	http_qdrive_info.c  \
	http_qdrvtypes.c  \
	http_qlsm.c  \
	http_qmedia.c  \
	http_qscratch.c  \
	http_qscr_mnt_info.c  \
	http_qscrpool_info.c  \
	http_qserver.c  \
	http_qvolume_info.c  \
	http_read_next.c \
	http_select_wait.c \
	http_set_scr.c \
	http_set_unscr.c \
	http_term.c    \
	http_volrpt.c    \
	http_work.c

OBJ     = $(SRC:.c=.o)

EXEC	= t_http

#
# Targets
#

all:    	nondebug install 

$(EXEC):	$(OBJ) $(XAPILIB) $(CLLIBS) $(APILIB)
	$(CC) -o $(EXEC) $(OBJ) $(CFLAGS) $(CLLIBS) $(XAPILIB) $(APILIB) $(LIBANSI) \
	$(UCBLIB_DIR) $(LIBRPCSOC) $(LIBSOCKET) $(LIBNSL) $(LIBPTHREADS) \
	$(LIBRUNPATH)



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
	rm -f  

sterile:
	rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#  Dependencies
#
#
