#
# Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
# 
#  Module:
# 
#	ACS Application Programmer's Interface Library
#
#	libapi.a 
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
# 
#  Modified by:
# 
#	Jim Montgomery  	10-May-1989	Original.
#	Howard Freeman  	08-Sep-1991	Updated for R3.0.
#	Howard I. Grapek	15-Oct-1992	Cleanup for toolkit.
#		Removed SQL, sb, lint refs, call to 'arch', 
#		calls to CPR. See NOTE above.  Install will now place
#		the library in ../../bin.
#       Ken Stickney     06-Nov-1993     Added BIN and COMPILER
#               variables, must be supplied on make command line. Added
#               "stripped" and "browse" targets. Dependency checking
#               now done with makedepend.
#       Ken Stickney     02-Dec-1993 	Transformed into simpler 
#		product makefile. Took out development options/functions.
#	H. I. Grapek	07-Jan-1994	Common_lib is libcl.a
#		Install library into ../../lib not ../../bin.  
#		removed BIN variable.  Miscellaneous cleanup for EC
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#	H. I. Grapek	14-Jan-1994	Documentation, cleanup.
#       Ken Stickney    20-Jan-1994     Added acs_type and acs_get_pktver to
#                                       Makefile
#       Ken Stickney    26-May-1994     Added acs_type_response, acs_state,
#                                       acs_command, and acs_status to
#                                       Makefile
#       Scott Siao      09-Oct-2001     Added acs_register, acs_unregister,
#                                       acs_chk_reg to Makefile
#       Scott Siao      12-Nov-2001     Added acs_display.
#       Scott Siao      29-Jan-2002     Added acs_query_ms_pinfo.c
#                                             acs_query_dg.c
#                                             acs_query_sn.c
#                                             acs_mount_pinfo.c
#	Hemendra(Wipro) 29-Dec-2002	Modified archiving commands for Linux
#					By default Linux does not come with 
#					lorder.
#       Mike Williams   27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                       enable the ability to change from
#                                       64-bit to 32-bit compile flags.
#                                       COMMON_CC_FLAGS is set in make.ini.
#                                       Updated the copyright statement.
#			

#       Definitions

#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

H 		=../h
INSTALL         =../../lib

#   Flags/Options
CC              = $(COMPILER)
CFLAGS          = $(COMMON_CC_FLAGS) -I$(H)
 
#    Libraries
 
APILIB 		=libapi.so
 
#   Target Variables

HDR	=$(H)/acsapi.h

SRC = \
	acs_audit_ac.c \
	acs_audit_ls.c \
	acs_audit_pa.c \
	acs_audit_se.c \
	acs_audit_su.c \
	acs_cancel.c \
	acs_clear_dr.c \
	acs_clear_vo.c \
	acs_command.c \
	acs_define_p.c \
	acs_delete_p.c \
	acs_dismount.c \
	acs_eject.c \
	acs_enter.c \
	acs_get_pktver.c \
	acs_idle.c \
	acs_lock_dr.c \
	acs_lock_vo.c \
	acs_mount.c \
	acs_mount_sc.c \
	acs_query_ac.c \
	acs_query_ca.c \
	acs_query_cl.c \
	acs_query_dr.c \
	acs_query_ld.c \
	acs_query_ls.c \
	acs_query_lv.c \
	acs_query_mm.c \
	acs_query_mo.c \
	acs_query_ms.c \
	acs_query_pl.c \
	acs_query_po.c \
	acs_query_re.c \
	acs_query_sc.c \
	acs_query_se.c \
	acs_query_vo.c \
	acs_response.c \
	acs_set_access.c \
	acs_set_cap.c \
	acs_set_clea.c \
	acs_set_scra.c \
	acs_start.c \
	acs_state.c \
	acs_status.c \
	acs_type.c \
	acs_type_resp.c \
	acs_unlock_dr.c \
	acs_unlock_vo.c \
	acs_vary_acs.c \
	acs_vary_cap.c \
	acs_vary_dri.c \
	acs_vary_lsm.c \
	acs_vary_por.c \
	acs_venter.c \
	acs_xeject.c \
	acs_register.c \
	acs_unregister.c \
	acs_chk_reg.c \
	acs_display.c \
	acs_query_dg.c \
	acs_query_sn.c \
	acs_mount_pinfo.c \
	acs_query_ms_pinfo.c 

OBJ  = $(SRC:.c=.o)
         
#
# Targets
#

all:    	nondebug install 

libapi.so:	$(OBJ)
	$(LD) -G $(LDFLAGS) -o $@ $?

libapi.a:  $(OBJ)
	if [ `uname` = "Linux" ]; \
	then 	ar cru $(APILIB) $(OBJ); \
	else 	ar cru $(APILIB) `lorder $(OBJ) | tsort`; \
	fi
	ranlib $(APILIB)
 
top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(APILIB) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(APILIB) CFLAGS="$(CFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"

install:	$(APILIB)
	cp -p $(APILIB) $(INSTALL)

clean:
	rm -f $(APILIB) $(OBJ) 

sterile:
	rm -f $(APILIB) $(OBJ) $(INSTALL)/$(APILIB)

#
#  Dependencies
#
