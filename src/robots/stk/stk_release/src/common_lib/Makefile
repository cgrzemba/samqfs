#
# Copyright (C) 1988,2011, Oracle and/or its affiliates. All rights reserved.
#
#  Module:
# 
#      Common Library Makefile
#
#      libcl.a
#
#      Note: This is a subset of the products common_lib.
# 
#  Environment: 
# 
#      Currently specifically for the sun, but tested on assorted other
#      releases of UNIX.
# 
#  Modified by:
# 
#      Ken Stickney        06-Nov-1993     Added BIN and COMPILER variables, 
#                                          must be supplied on make command line. 
#                                          Added "stripped" and "browse" targets. 
#                                          Dependency checking now done with makedepend.
#      Han Shum            11-Nov-1993     Removed all the modules that
#                                          are not used by the toolkit.
#      Ken Stickney        02-Dec-1993     Transformed into simpler product makefile. 
#                                          Took out development options/functions.
#      H. I. Grapek        07-Jan-1994     Common_lib is libcl.a
#                                          Install library into ../../lib not ../../bin.  
#                                          removed BIN variable.  
#                                          Miscellaneous cleanup for EC
#                                          Added inclusion of make.ini
#                                          Removed DEBUG and NONDEBUG variables 
#                                          (they are in make.ini)
#      H. I. Grapek        14-Jan-1994     Documentation, cleanup.
#      S. L. Siao          29-Oct-2001     Added cl_reply_type.c, cl_resource.c
#      Hemendra(Wipro)     29-Dec-2002     Modified archiving commands for Linux
#                                          By default Linux does not come with lorder.
#      Mitch Black         01-Dec-2004     Cleaned up - See diffs in Continuus.
#      Mike Williams       27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                          enable the ability to change from
#                                          64-bit to 32-bit compile flags.
#                                          COMMON_CC_FLAGS is set in make.ini.
#                                          Updated the copyright statement.
#                                          Added CFLAGS to CC statement.
#      Mike Williams       05-Jan-2011     Updated to remove the check for Linux prior to
#                                          running ar command.
#      Joseph Nofi         15-May-2011     Added srvcommon.h to header files.
#                                          Added new srvlogs, srvtrcs, and srvvars objects.
#

#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

#	Definitions

H		=../h 
INSTALL		=../../lib

#	Flags/Options

CC              = $(COMPILER)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) -I../h/api

#	Libraries

CLLIBS		=../../lib/libcl.a

#	Target Variables

HDR   = $(H)/cl_qm_defs.h \
	$(H)/cl_sm_defs.h \
	$(H)/db_defs.h \
	$(H)/db_structs.h \
	$(H)/defs.h \
	$(H)/identifier.h \
	$(H)/structs.h \
	$(H)/srvcommon.h

SRC   =	\
	cl_chk_input.c \
	cl_command.c \
	cl_defs.c \
	cl_el_log.c \
	cl_el_log_register.c \
	cl_get_sockn.c \
	cl_identifie.c \
	cl_inform.c \
	cl_ipc_creat.c \
	cl_ipc_destr.c \
	cl_ipc_open.c \
	cl_ipc_read.c \
	cl_ipc_send.c \
	cl_ipc_write.c \
	cl_ipc_xmit.c \
	cl_log.c \
	cl_log_unexp.c \
	cl_proc_init.c \
	cl_qm_create.c \
	cl_qm_data.c \
	cl_qm_find.c \
	cl_qm_init.c \
	cl_qm_macces.c \
	cl_qm_mcreat.c \
	cl_qm_mdelet.c \
	cl_qm_mlocat.c \
	cl_qm_mstatu.c \
	cl_qm_qcreat.c \
	cl_qm_qdelet.c \
	cl_select_in.c \
	cl_sig_desc.c \
	cl_sig_hdlr.c \
	cl_sig_trap.c \
	cl_state.c \
	cl_status.c \
	cl_str_to_buf.c \
	cl_trace.c \
	cl_type.c \
	dv_api.c \
	ml_api.c \
	cl_resource.c \
	cl_reply_type.c \
	srvlogc.c \
	srvtrcc.c \
	srvvars.c 

OBJ  = $(SRC:.c=.o)			# generate C object list

EXEC  = libcl.so

#
# Targets
#

all:    	nondebug install 

libcl.so:      $(OBJ)
	$(LD) -G $(LDFLAGS) -o $@ $?


libcl.a:	$(OBJ)
#	if [ `uname` = "Linux" ]; \
#	then	ar cru libcl.a $(OBJ); \
#	else	ar cru libcl.a `lorder $(OBJ) | tsort`; \
#	fi
	ar cru libcl.a $(OBJ)
	ranlib libcl.a

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	$(MAKE) -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)" 

nondebug:
	$(MAKE) -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"

install:	$(EXEC)
	cp -p $(EXEC) $(INSTALL)

clean:
	rm -f $(OBJ) $(EXEC)

sterile:
	rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#  Dependencies
#
