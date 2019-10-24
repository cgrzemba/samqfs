#
# Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
#
#  Name:
# 
#	CSI makefile.
#
#  Environment: 
# 
#       Tested for Sun SOLARIS, IBM AIX, and RedHat Linux 8.0.
#       Can be made to run on other releases of UNIX and non-UNIX
#       OS's by STK partners.
#
#  Modified by:
# 
#       J. A. Wishner           23-Jan-1989     Created..
#       R. P. Cushman           08-May-1990     Added lines to compile pd mods
#       J. S. Alexander         29-May-1990     Added STATS to CFLAGS.
#       J. W. Montgomery        18-Jun-1990     Version 2.
#       J. A. Wishner           23-Sep-1990     Modified to split out xdr funcs.
#       Howard I. Grapek        29-Aug-1991     Mods needed for R3 (VERSION2)
#       Howard L. Freeman IV    15-Sep-1991     Mods for R3 ACSAPI/CSI Test tape
#       J. A. Wishner           03-Oct-1990     Completed mods for R-3.
#       D. A. Beidle            03-Dec-1991.    Added csi_chk_input module as a
#		substitute for the cl_chk_input module.
#	Howard I. Grapek	15-Oct-1992	Cleanup for toolkit.
#		Removed SQL, sb, lint refs, calls to CPR. 
#		See NOTE above.  Also removed t_* and placed in separate 
#		directory in tree. 
#       Ken Stickney            06-Nov-1993     Added BIN and COMPILER
#               variables, must be supplied on make command line. Added
#               "stripped" and "browse" targets. Dependency checking
#               now done with makedepend.
#       Ken Stickney            02-Dec-1993     Transformed into simpler
#               product makefile. Took out development options/functions.
#	H. I. Grapek		07-Jan-1994	Common_lib is libcl.a
#		Removed libdv, removed BIN variable.  
#		Miscellaneous cleanup for EC
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       Ken Stickney    23-Dec-1994     Libarary flag changes for Solaris.
#       Mitch Black     01-Dec-2004     Cleaned up - see diffs in Continuus.
#       Mitch Black     25-Aug-2003     Added LIBRUNPATH for runtime path of
#                                       shared object libraries (for Solaris).
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

#	Definitions

H		=../h
INSTALL		=../../bin

#	Flags/Options

CC              = $(COMPILER)
ADIFLAGS        = $(ADI_FLAGS)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H) $(ADIFLAGS)

#	Libraries

CLLIBS		=../../lib/libcl.a
XDRLIBS		=../../lib/CSIXDR.a
ADILIBS  	= $(ADI_LIBS)

SRC   = csi_main.c \
	csi_chk_inpu.c \
	csi_fmtlmq_l.c \
	csi_fmtniq_l.c \
	csi_freeqmem.c \
	csi_getiaddr.c \
	csi_getmsg.c \
	csi_globals.c  \
	csi_hostaddr.c \
	csi_init.c \
	csi_ipcdisp.c  \
	csi_logevent.c \
	csi_net_send.c \
	csi_netbufin.c \
	csi_process.c \
	csi_ptrace.c \
	csi_qclean.c \
	csi_qcmp.c \
	csi_qinit.c \
	csi_qget.c \
	csi_qput.c \
	csi_rpcinput.c \
	csi_rpctrans.c \
	csi_shutdown.c \
	csi_sighdlr.c \
	csi_ssicmp.c \
	csi_svcinit.c \
	csi_rpccall.c \
	csi_rpcdisp.c \
	csi_rpctinit.c \
	csi_rpcuinit.c 

OBJ  	= $(SRC:.c=.o)

ADISRC = $(ADI_FILE_LIST)

ADIOBJ = $(ADISRC:.c=.o)

EXEC	= csi      

#
# Targets
#

all:    	nondebug install 

$(EXEC): $(OBJ) $(ADIOBJ) $(XDRLIBS) $(CLLIBS)
	$(CC) -o $@ $(OBJ) $(ADIOBJ) $(CFLAGS) $(LIBRUNPATH) $(XDRLIBS) \
	$(CLLIBS) $(ADILIBS) $(LIB_DIR) $(UCBLIB_DIR) $(LIBRPCSOC) \
	$(LIBSOCKET) $(LIBNSL)

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"

install: $(EXEC)
	cp $(EXEC) $(INSTALL)

clean:
	-rm -fr $(EXEC) $(OBJ)

sterile:
	-rm -fr $(EXEC) $(OBJ) $(INSTALL)/$(EXEC)

#
#	Dependencies
#
