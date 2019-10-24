#
# Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
#
#  Module:
# 
#	ACS Application Programmer's Machine Dependent Interface Library
#
#	acsipc.a 
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
# 
#  Modified by:
# 
#	Ken Stickney  	10-May-1993	Original.
#       Ken Stickney  	06-Nov-1993     Added BIN and COMPILER
#                variables, must be supplied on make command line. Added
#                "stripped" and "browse"  targets. Dependency checking 
#                now done with makedepend. 
#       Ken Stickney  	02-Dec-1993     Transformed into simpler
#                product makefile. Took out development options/functions.
#	H. I. Grapek	07-Jan-1994	Common_lib is libcl.a
#		Install library into ../../lib not ../../bin.  
#		removed BIN variable.  Miscellaneous cleanup for EC
# 		Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       K. Stickney     20-Jan_1994     Removed acs_get_pktver.c     
#       Hemendra(Wipro) 29-Dec-2002     Modified archiving commands for Linux
#                                       By default Linux does not come with 
#					lorder.
#       Mike Williams   27-Apr-2010     Updated to include COMMON_CC_FLAGS to
#                                       enable the ability to change from
#                                       64-bit to 32-bit compile flags.
#                                       COMMON_CC_FLAGS is set in make.ini.
#                                       Updated the copyright statement.
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
 
IPCLIB 		=libipc.so
 
#   Target Variables

HDR	=$(H)/acsapi.h

SRC = \
        acs_error.c \
	acs_get_sock.c \
	acs_ipc_read.c \
	acs_ipc_write.c \
	acs_sel_input.c \
	acs_verify_ssi.c

OBJ  = $(SRC:.c=.o)
         
#
# Targets
#

all:    	nondebug install 

libipc.so:      $(OBJ)
	$(LD) -G $(LDFLAGS) -o $@ $?

libipc.a:  $(OBJ)
	if [ `uname` = "Linux" ]; \
	then	ar cru $(IPCLIB) $(OBJ); \
	else	ar cru $(IPCLIB) `lorder $(OBJ) | tsort`; \
	fi
	ranlib $(IPCLIB)
 
top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(IPCLIB) CFLAGS="$(CFLAGS) $(DEBUG) $(ANSI_INCLUDE)"

nondebug:
	make -f Makefile $(IPCLIB) CFLAGS="$(CFLAGS) $(NONDEBUG) \
	$(ANSI_INCLUDE)"

install:	$(IPCLIB)
	cp -p $(IPCLIB) $(INSTALL)

clean:
	rm -f $(IPCLIB) $(OBJ)

sterile:
	rm -f $(IPCLIB) $(OBJ) $(INSTALL)/$(IPCLIB)

#
#  Dependencies
#
