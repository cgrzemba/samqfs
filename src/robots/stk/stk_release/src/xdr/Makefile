#
# Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
# 
#  Name:
# 
#	RPC CSI/SSI XDR Library makefile.
#
#	CSIXDR.a
#	SSIXDR.a	(sources conditionally compiled with -DSSI)
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
# 
#  Modified by:
# 
#       J. A. Wishner  	23-Jan-1989     Created..
#       R. P. Cushman  	08-May-1990     Added lines to compile pd modules
#       J. S. Alexander 29-May-1990     Added STATS to CFLAGS.
#       J. Montgomery  	18-Jun-1990     Version 2.
#       J. A. Wishner   23-Sep-1990     Modified to split out xdr funcs.
#       H. I. Grapek  	29-Aug-1991     Mods needed for R3 (VERSION2)
#       H. L. Freeman	15-Sep-1991     Mods for R3 ACSAPI/CSI Test tape
#       J. A. Wishner 	03-Oct-1990     Completed mods for R-3.
#       D. A. Beidle    03-Dec-1991.    Added csi_chk_input module as a
#		substitute for the cl_chk_input module.
#	H. I. Grapek	15-Oct-1992	Cleanup for toolkit.
#		Removed SQL, sb, lint refs, calls to CPR. 
#		See NOTE above.  Also removed t_* and placed in separate 
#		directory in tree.   Pulled out from CSI,SSI
#       K. Stickney     06-Nov-1993     Added BIN and COMPILER
#      		variables, must be supplied on make command line. Added
#               "stripped" and "browse" targets. Dependency checking
#               now done with makedepend.
#       K. Stickney 	02-Dec-1993     Transformed into simpler
#		product makefile. Took out development options/functions.
#	H. I. Grapek	07-Jan-1994	Cleaned up clean and install
#		Common_lib is libcl.a
#		Install libraries into ../../lib not ../../bin.  
#		removed BIN variable.  
#		Miscellaneous cleanup for EC
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#	H. I. Grapek	13-Jan-1994	Made a rm -f when 
#					removing the libs from the lib place.
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       K. Stickney     23-Dec-1994     Changes to solve order dependent
#                                       sensitivity to builds.
#       Chris Higgins   01-Nov-2001     Added csi_xev_reg_st.c  
#                                             csi_xsense_fsc.c  
#                                             csi_xev_rsrc_st.c  
#                                             csi_xregister_st.c  
#                                             csi_xsense_hli.c 
#                                             csi_xev_vol_st.c 
#                                             csi_xresrc_dta.c  
#                                             csi_xsense_scsi.c 
#                                             csi_xhand_id.c  
#                                             csi_xrgstrn_id.c 
#                                             csi_xserial_num.c
#       Chris Higgins   12-Nov-2001     Added csi_xxml_data.c  
#       Scott Siao      04-Feb-2002     Added csi_xgrp_type.c  
#       Hemendra(Wipro) 29-Dec-2002     Modified archiving commands for Linux
#                                       By default Linux does not come with 
#					lorder.
#       Wipro (Subhash) 04-Jun-2004     Added csi_xev_drive_st.c and
#						csi_xdrive_data.c
#	Mitch Black	24-Nov-2004	Added in csi_xptp_id.c
#       Mike Williams   27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                       enable the ability to change from 
#                                       64-bit to 32-bit compile flags.
#                                       COMMON_CC_FLAGS is set in make.ini.
#                                       Updated the copyright statement.

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

SSIFLAGS	=-DSSI
ADIFLAGS        = $(ADI_FLAGS)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) $(ADIFLAGS)

#	Libraries

CLLIBS		= -L $(OBJ_DIR) -lcl
XDRLIBS		= libcsixdr.so libssixdr.so
CSIXDR          = libcsixdr.so
SSIXDR          = libssixdr.so

#	Target Variables

SRC = \
	csi_xaccess.c  \
	csi_xacs.c     \
	csi_xcap.c     \
	csi_xcap_id.c  \
	csi_xcap_mod.c \
	csi_xcell_id.c \
	csi_xcol.c     \
	csi_xcommand.c \
	csi_xcsi_hdr.c \
	csi_xdrive.c   \
	csi_xdrive_i.c \
	csi_xdrive_t.c \
	csi_xfreecel.c \
	csi_xidcmp.c \
	csi_xidentif.c \
	csi_xipc_hdr.c \
	csi_xlm_requ.c \
	csi_xlm_resp.c \
	csi_xlocatio.c \
	csi_xlockid.c  \
	csi_xlsm.c     \
	csi_xlsm_id.c  \
	csi_xmedia_t.c \
	csi_xmsg_hdr.c \
	csi_xmsg_id.c  \
	csi_xpnl.c     \
	csi_xpnl_id.c  \
	csi_xpool.c    \
	csi_xpool_id.c \
	csi_xport.c    \
	csi_xport_id.c \
	csi_xptp_id.c \
	csi_xqu_res.c  \
	csi_xreq_hdr.c \
	csi_xreqsumm.c \
	csi_xres_sta.c \
	csi_xrow.c     \
	csi_xsocknam.c \
	csi_xspnl_id.c \
	csi_xstate.c   \
	csi_xstatus.c  \
	csi_xtype.c    \
	csi_xversion.c \
	csi_xvol_id.c  \
	csi_xvolrang.c \
	csi_xvol_sta.c \
	csi_xv0qures.c \
	csi_xv0_cap_.c \
	csi_xv0_req.c  \
	csi_xv0_res.c  \
	csi_xv1qures.c \
	csi_xv1_cap_.c \
	csi_xv1_req.c  \
	csi_xv1_res.c  \
	csi_xv2qures.c \
	csi_xv2_req.c  \
	csi_xv2_res.c  \
	csi_xv4_req.c  \
	csi_xv4_res.c  \
	csi_xev_reg_st.c  \
	csi_xsense_fsc.c  \
	csi_xev_rsrc_st.c  \
	csi_xregister_st.c  \
	csi_xsense_hli.c \
	csi_xev_vol_st.c \
	csi_xresrc_dta.c  \
	csi_xsense_scsi.c \
	csi_xhand_id.c  \
	csi_xrgstrn_id.c \
	csi_xserial_num.c \
	csi_xxml_data.c \
	csi_xgrp_type.c \
	csi_xev_drive_st.c \
	csi_xdrive_data.c 

# Specific for the SSI
SSISRC= csi_xlm_requ.c \
	csi_xlm_resp.c

OBJ  	= $(SRC:.c=.o) 	
SSIOBJ  = $(SSISRC:.c=.o) 

#
# Targets
#

all:    	nondebug install 

libcsixdr.so: 	$(OBJ)
	$(LD)	-G -o $@ $?

CSIXDR.a:	$(OBJ)
	if [ `uname` = "Linux" ]; \
	then	ar cru CSIXDR.a $(OBJ); \
	else	ar cru CSIXDR.a `lorder $(OBJ) | tsort`; \
	fi
	ranlib CSIXDR.a

libssixdr.so: 	$(SSIOBJ)
	$(LD)	-G -o $@ $?

SSIXDR.a:	$(SSIOBJ)
	if [ `uname` = "Linux" ]; \
	then	ar cru SSIXDR.a $(OBJ); \
	else	ar cru SSIXDR.a `lorder $(OBJ) | tsort`; \
	fi
	ranlib SSIXDR.a

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile CSIXDR.a CFLAGS="$(CFLAGS) $(DEBUG) \
	$(ANSI_INCLUDE)"
	rm -f $(SSIOBJ)
	make -f Makefile SSIXDR.a CFLAGS="$(CFLAGS) $(SSIFLAGS) $(DEBUG) \
	$(ANSI_INCLUDE)"
	rm -f $(SSIOBJ)

nondebug:
	make -f Makefile $(CSIXDR) CFLAGS="$(CFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"
	rm -f $(SSIOBJ)
	make -f Makefile $(SSIXDR) CFLAGS="$(CFLAGS) $(SSIFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"
	rm -f $(SSIOBJ)


install: 
	cp -p  $(XDRLIBS) $(INSTALL)

clean:
	rm -f $(OBJ) $(XDRLIBS) 

sterile:
	rm -f $(OBJ) $(XDRLIBS) $(INSTALL)/$(CSIXDR) $(INSTALL)/$(SSIXDR)

#
#	Dependencies
#
