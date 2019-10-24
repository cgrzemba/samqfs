#
# Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
# 
#  Name:
#	Makefile
#
#  Description: 
#	Makefile for building the Trace Decoder
# 
#  Environment: 
#	NONE
#
#  Modified by:
# 
#	M. H. Shum              14-Sep-1993     Original
#	H. I. Grapek		07-Jan-1994	Common_lib is libcl.a
#		Removed libdv, removed BIN variable.  
#		Miscellaneous cleanup for EC
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       K. Stickney     24-Dec-1994     Library flag changes for
#                                       Solaris.
#       S. L. Siao      01-Nov-2001     Added td_register_p.c
#	                                      td_register_q.c 
#	                                      td_unregister_p.c 
#	                                      td_unregister_q.c 
#	                                      td_chk_reg_p.c 
#	                                      td_chk_reg_q.c
#       S. L. Siao      12-Nov-2001     Added td_display_p.c,td_display_q.c
#	Mitch Black	13-Dec-2004	Removed $LIBRPCSOC and $LIBSOCKET
#					from trace_decode link operation.
#	Mitch Black	14-Dec-2004	Removed $CFLAGS from trace_decode
#					link operation (it could cause 
#					unnecessary "invalid option" errors
#					when linking (e.g. under AIX).
#	Mitch Black	04-Apr-2005	Put $LIBRPCSOC and $LIBSOCKET back
#					in the link line.  These are needed
#					ONLY for the Solaris DEBUG version,
#					and Solaris is the only OS for which 
#					these libs are defined in make.ini.
#       Mike Williams   27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                       enable the ability to change from 
#                                       64-bit to 32-bit compile flags.
#                                       COMMON_CC_FLAGS is set in make.ini.
#                                       Updated the copyright statement.
#					Added CFLAGS to CC statement.
#

#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

# Defines

H	=../h
INSTALL	=../../bin

#	Flags/Options

CC	=$(COMPILER)
CFLAGS	= $(COMMON_CC_FLAGS) -I$(H) 

#	Libraries

CLLIB	=../../lib/libcl.a

#	Target Variables
 
SRC =	td_main.c \
	td_get_packe.c \
	td_packet.c \
	td_cr_info.c \
	td_csi_head.c \
	td_msg_head.c \
	td_req.c \
	td_audit_q.c \
	td_cancel_q.c \
	td_dismoun_q.c \
	td_eject_q.c \
	td_enter_q.c \
	td_mount_q.c \
	td_query_q.c \
	td_vary_q.c \
	td_set_scr_q.c \
	td_def_pl_q.c \
	td_del_pl_q.c \
	td_set_cln_q.c \
	td_mount_s_q.c \
	td_lock_q.c \
	td_set_cap_q.c \
	td_resp.c \
	td_audit_p.c \
	td_cancel_p.c \
	td_dismoun_p.c \
	td_mount_p.c \
	td_query_p.c \
	td_vary_p.c \
	td_set_scr_p.c \
	td_def_pl_p.c \
	td_del_pl_p.c \
	td_set_cln_p.c \
	td_mount_s_p.c \
	td_lock_p.c \
	td_qu_lock_p.c \
	td_set_cap_p.c \
	td_eject_ent.c \
	td_idle_com.c \
	td_start_com.c \
	td_ack.c \
	td_convert.c \
	td_print_id.c \
	td_pr_misc.c \
	td_print.c \
	td_hex_dump.c  \
	td_register_p.c \
	td_register_q.c \
	td_unregister_p.c \
	td_unregister_q.c \
	td_chk_reg_p.c \
	td_chk_reg_q.c \
	td_display_p.c \
	td_display_q.c \
	td_mount_pinfo_p.c \
	td_mount_pinfo_q.c

OBJ	= $(SRC:.c=.o)

EXEC	= trace_decode

#
# Targets
#

all:    	nondebug install 

$(EXEC):	$(OBJ) $(CLLIB)
	$(CC) -o $(EXEC) $(OBJ) $(CFLAGS) $(CLLIB) $(LIBANSI) \
	$(UCBLIB_DIR) $(LIBRPCSOC) $(LIBSOCKET) $(LIBNSL) \
	$(LIBRUNPATH)

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) $(ANSI_INCLUDE)"

install:	$(EXEC)
	cp  $(EXEC) $(INSTALL)

clean:
	-rm -f $(EXEC) $(OBJ) 

sterile:
	-rm -f $(EXEC) $(OBJ) $(INSTALL)/$(EXEC)

#
#	Dependencies
#
