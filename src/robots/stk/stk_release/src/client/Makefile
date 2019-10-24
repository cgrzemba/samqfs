#
# Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
#
# 
#  Name:
# 
#	Toolkit Driver Makefile
#
#	t_cdriver	- an acsapi exerciser.
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
#
#  Modified by:
# 
#       J. A. Wishner           23-Jan-1989     Created..
#       R. P. Cushman           08-May-1990     Added lines to compile pd 
#       J. S. Alexander         29-May-1990     Added STATS to CFLAGS.
#       J. W. Montgomery        18-Jun-1990     Version 2.
#       J. A. Wishner           23-Sep-1990     Modified to split out xdr funcs.
#       Howard I. Grapek        29-Aug-1991     Mods needed for R3 (VERSION2)
#       Howard L. Freeman IV    15-Sep-1991     Mods for R3 ACSAPI/CSI tape
#       J. A. Wishner           03-Oct-1990     Completed mods for R-3.
#       D. A. Beidle            03-Dec-1991.    Added csi_chk_input module as 
#		substitute for the cl_chk_input module.
#	Howard I. Grapek	15-Oct-1992	Cleanup for toolkit.
#		Removed SQL, sb, lint refs, call to 'arch', 
#		calls to CPR. See NOTE above.  Moved from the CSI directory.
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
#       Ken Stickney    23-Dec-1994     Added various library flags for 
#                                       solaris portability.
#       Ken Stickney    26-Jan-1995     Fixed flags.
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
INSTALL         =../../bin

#	Flags/Options

CC              = $(COMPILER)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H)

#	Libraries

APILIB		=../../lib/libapi.a
IPCLIB          =../../lib/libipc.a
UTLLIB		=../../lib/libutl.a
CLLIB		=../../lib/libcl.a 
#	Target Variables 
SRC   		= t_cdriver.c
OBJ  		= $(SRC:.c=.o)
EXEC    	= t_cdriver

#
# Targets
#

all:    	nondebug install 

t_cdriver: t_cdriver.o $(APILIB) $(IPCLIB) $(UTLLIB) $(IPCLIB) $(CLLIB) 
	$(CC) -o $@ t_cdriver.o $(CFLAGS) $(APILIB) $(UTLLIB) $(IPCLIB) \
        $(CLLIB) $(LIB_DIR) $(LIBSOCKET) $(LIBNSL)

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG) $(ANSI_INCLUDE)" 

install: $(EXEC) 
	cp $(EXEC) $(INSTALL)

clean:
	-rm -f $(OBJ) $(EXEC)

sterile:
	-rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#	Dependencies
#
