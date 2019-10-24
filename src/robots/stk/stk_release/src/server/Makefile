#
# Copyright (C) 1992,2010, Oracle and/or its affiliates. All rights reserved.
#
# 
#  Name:
# 
#	Toolkit Tools and Drivers Makefile
#
#	t_acslm 	- an acslm stub
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
#
#  Modified by:
# 
#       Ken Stickney	06-Nov-1993     Added BIN and COMPILER
#      		variables, must be supplied on make command line. Added
#  		"stripped" and "browse" targets. Dependency checking
#		now done with makedepend.
#
#       Ken Stickney 	02-Dec-1993     Transformed into simpler
#       	product makefile. Took out development options/functions.
#
#	Howie Grapek	04-Jan-1994	Removed DV LIB.
#
#	H. I. Grapek		07-Jan-1994	Common_lib is libcl.a
#		Removed libdv, removed BIN variable.  
#		Miscellaneous cleanup for EC
#		include the makesun.sh
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       K. J. Stickney  05-May_1994     Added conversion routines for 
#                                       servicing down level packets.
#       Ken Stickney  05-Jan_1995       Added library flags for Solaris.
#       Ken Stickney  26-Jan_1995       Fixed library flags for Solaris.
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
INSTALL	        =../../bin

#	Flags/Options

CC              = $(COMPILER)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H)

#	Libraries

APILIB		=../../lib/libapi.a
IPCLIB          =../../lib/libipc.a
UTLLIB          =../../lib/libutl.a
CLLIB           =../../lib/libcl.a

#	Target Variables

SRC   = t_acslm.c \
	cvt_resp.c \
	cvt_req.c \
	cvt_v0_v1.c \
	cvt_v1_v0.c \
	cvt_v2_v1.c \
	cvt_v1_v2.c \
	cvt_v3_v2.c \
	cvt_v2_v3.c \
	cvt_v3_v4.c \
	cvt_v4_v3.c

OBJ  		= $(SRC:.c=.o)
EXEC    	= t_acslm

#
# Component Targets
#

all:    	nondebug install 

t_acslm: $(OBJ) $(CLLIB)
	$(CC) -o $@ $(OBJ) $(CFLAGS) $(CLLIB) $(LIB_DIR) $(LIBSOCKET) $(LIBNSL)

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG)"

nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG)" 

install: $(EXEC) 
	cp $(EXEC) $(INSTALL)

clean:
	-rm -f $(OBJ) $(EXEC)

sterile:
	-rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
#	Dependencies
#
