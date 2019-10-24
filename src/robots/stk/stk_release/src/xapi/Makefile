# SccsId @(#)Makefile            7.2 05/11/11 
#                                                               
# Copyright (c) 2011, 2011, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      Makefile                                      
#                                                               
# Description:    XAPI libxapi.a library makefile                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720/CDK240  Joseph Nofi     05/11/11                       
#     Created for CDK to add XAPI support.
#                                                               
#===============================================================
#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

#	Definitions

H		=../h 
INSTALL		=../../lib

#	Flags/Options
DEFINE		=-DXAPI
CC              = $(COMPILER)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) -I../h/api -I../h/xapi $(DEFINE) 

#	Libraries

CLLIBS		=../../lib/libcl.a
APILIB		=../../lib/libapi.a
XAPILIB		=../../lib/libxapi.a

#	Target Variables

HDR=	$(H)/csi.h \
	$(H)/csi_structs.h \
	$(H)/csi_v0_structs.h \
	$(H)/csi_v1_structs.h \
	$(H)/srvcommon.h 

SRC   =	\
	smccxcs.c \
	smccxcv.c \
	smccxfe.c \
	smccxml.c \
	smccxmv.c \
	smccxps.c \
	smccxtk.c \
	smcsasc.c \
	smcsfmt.c \
	smcstok.c \
	xapi_attach.c \
	xapi_audit.c \
	xapi_cancel.c \
	xapi_check_reg.c \
	xapi_clr_lock.c \
	xapi_clr_lock_drv.c \
	xapi_clr_lock_vol.c \
	xapi_config.c \
	xapi_config_search.c \
	xapi_define_pool.c \
	xapi_delete_pool.c \
	xapi_dismount.c \
	xapi_display.c \
	xapi_drive.c \
	xapi_drive_list.c \
	xapi_drvtyp.c \
	xapi_drvtyp_search.c \
	xapi_eject.c \
	xapi_enter.c \
	xapi_idle.c \
	xapi_idle_test.c \
	xapi_lock.c \
	xapi_lock_drv.c \
	xapi_lock_init_resp.c \
	xapi_lock_vol.c \
	xapi_main.c \
	xapi_media.c \
	xapi_media_search.c \
	xapi_mount.c \
	xapi_mount_pinfo.c \
	xapi_mount_scr.c \
	xapi_parse_common.c \
	xapi_qacs.c \
	xapi_qcap.c \
	xapi_qdrv.c \
	xapi_qdrv_group.c \
	xapi_qfree.c \
	xapi_qlock.c \
	xapi_qlock_drv.c \
	xapi_qlock_drv_one.c \
	xapi_qlock_init_resp.c \
	xapi_qlock_vol.c \
	xapi_qlock_vol_one.c \
	xapi_qlsm.c \
	xapi_qmedia.c \
	xapi_qmnt_one.c \
	xapi_qmnt_pinfo.c \
	xapi_qmnt_scr.c \
	xapi_qmnt.c \
	xapi_qpool.c \
	xapi_qrequest.c \
	xapi_qscr.c \
	xapi_qserver.c \
	xapi_qsubpool.c \
	xapi_query.c \
	xapi_query_init_resp.c \
	xapi_qvol.c \
	xapi_qvol_all.c \
	xapi_qvol_one.c \
	xapi_register.c \
	xapi_request_header.c \
	xapi_response.c \
	xapi_scrpool.c \
	xapi_scrpool_counts.c \
	xapi_scrpool_search.c \
	xapi_set_cap.c \
	xapi_set_clean.c \
	xapi_set_scr.c \
	xapi_start.c \
	xapi_tcp.c \
	xapi_unlock.c \
	xapi_unlock_drv.c \
	xapi_unlock_vol.c \
	xapi_unregister.c \
	xapi_userid.c \
	xapi_vary.c \
	xapi_venter.c \
	xapi_volser.c \
	xapi_xeject.c

OBJ     = $(SRC:.c=.o)

EXEC  = libxapi.a

#
# Targets
#

all:    	nondebug install 

libxapi.a:	$(OBJ)
#	if [ `uname` = "Linux" ]; \
#	then	ar cru libxapi.a $(OBJ); \
#	else	ar cru libxapi.a `lorder $(OBJ) | tsort`; \
#	fi
	ar cru libxapi.a $(OBJ)
	ranlib libxapi.a

top_debug:	debug install 
top_nondebug:	nondebug install 
top_sterile:	sterile

debug:
	$(MAKE) -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)" 

nondebug:
	$(MAKE) -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) $(ANSI_INCLUDE)"

install:	$(EXEC)
	cp -p $(EXEC) $(INSTALL)

clean:
	rm -f $(TEMPI) $(TEMPS) 

sterile:
	rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#  Dependencies
#
