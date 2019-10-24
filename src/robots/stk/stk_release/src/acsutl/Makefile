#
# Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
#
#  Module:
#
#       ACSAPI Application Programmer's Uility Library
#
#       libutl.a
#
#  Environment:
#
#       Currently specifically for the sun, but tested on assorted other
#       releases of UNIX.
#
#  Modified by:
#
#       Ken Stickney            10-May-1993     Original.
#       Ken Stickney            06-Nov-1993     Added BIN and COMPILER
#               variables, must be supplied on make command line. Added
#               "stripped" and "browse"  targets. Dependency checking
#               now done with makedepend.
#       Ken Stickney            02-Dec-1993     Transformed into simpler
#               product makefile. Took out development options/functions.
#       H. I. Grapek    07-Jan-1994     Common_lib is libcl.a
#               Install library into ../../lib not ../../bin.
#               removed BIN variable.  Miscellaneous cleanup for EC
#               Added inclusion of make.ini
#               Removed DEBUG and NONDEBUG variables (they are in make.ini)
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       K. Stickney     20-Jan-1994     Removed acs_type from Makefile
#       K. Siickney     06-May-1994     Added all of the new packet conversion
#                                       functions for support of down level
#                                       servers:
#                                       acs_cvt_v1_v2.c
#                                       acs_cvt_v2_v3.c
#                                       acs_cvt_v3_v4.c
#                                       acs_cvt_v4_v3.c
#                                       acs_cvt_v3_v2.c
#                                       acs_cvt_v2_v1.c
#       K. Siickney     06-Aug-1994     Removed acs_cvt_v1_v2.c & 
#                                       acs_cvt_v2_v1.c
#
#       S. Siao         01-Nov-2001     Added acs_resp_regi.c
#       Hemendra(Wipro) 29-Dec-2002     Modified archiving commands for Linux
#                                       By default Linux does not come with 
#					lorder.
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
 
#       Definitions
 
H               =../h
INSTALL         =../../lib
 
#   Flags/Options
 
CC              = $(COMPILER)
CFLAGS          = $(COMMON_CC_FLAGS) -I$(H)
 
#    Libraries
 
UTLLIB          =libutl.so
 
#   Target Variables
 
HDR     =$(H)/acsapi.h
 
SRC = \
	acs_build_hdr.c \
	acs_cvt_v2_v3.c \
	acs_cvt_v3_v4.c \
	acs_cvt_v4_v3.c \
	acs_cvt_v3_v2.c \
	acs_get_resp.c \
	acs_query_res.c \
	acs_resp_audf.c \
	acs_resp_audi.c \
	acs_resp_regi.c \
	acs_send_req.c \
	acs_vary_res.c
 
OBJ  = $(SRC:.c=.o)
 
#
# Targets
#
 
all:            nondebug install
 
libutl.so:      $(OBJ)
	$(LD) -G $(LDFLAGS) -o $@ $?

libutl.a:  $(OBJ)
	if [ `uname` = "Linux" ]; \
	then	ar cru $(UTLLIB) $(OBJ); \
	else	ar cru $(UTLLIB) `lorder $(OBJ) | tsort`; \
	fi
	ranlib $(UTLLIB)
 
top_debug:      debug install
top_nondebug:   nondebug install
top_sterile:    sterile
 
 
debug:
	make -f Makefile $(UTLLIB) CFLAGS="$(CFLAGS) $(DEBUG)"
 
nondebug:
	make -f Makefile $(UTLLIB) CFLAGS="$(CFLAGS) $(NONDEBUG)"
 
install:        $(UTLLIB)
	cp -p $(UTLLIB) $(INSTALL)
 
clean:
	rm -f $(UTLLIB) $(OBJ)
 
sterile:
	rm -f $(UTLLIB) $(OBJ) $(INSTALL)/$(UTLLIB)
 
#
#  Dependencies
#

