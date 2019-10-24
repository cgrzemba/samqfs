#
# Copyright (C) 1994,2010, Oracle and/or its affiliates. All rights reserved.
# 
#  Module:
# 
#	Mini Event Logger makefile.
#
#	mini_el
#
#  Environment: 
# 
#	Currently specifically for the sun, but tested on assorted other
#	releases of UNIX.
# 
#  Modified by:
# 
#	Howie Grapek	04-Jan-1994	Original
#	Howie Grapek	09-Jan-1994	Added inclusion of make.ini
#		Removed DEBUG and NONDEBUG variables (they are in make.ini)
#       H. I. Grapek    14-Jan-1994     Documentation, cleanup.
#       Ken Stickeny    23-Dec-1994     Added library flags for Solaris.
#       Ken Stickeny    23-Dec-1994     Fixed library flags for Solaris.
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

#    Definitions

H		=../h
INSTALL		=../../bin
 
#   Flags/Options
 
CC              = $(COMPILER)
CFLAGS		= $(COMMON_CC_FLAGS) -I$(H)
 
#    Libraries
 
#   Target Variables

EXEC = mini_el
SRC  = mini_el.c
OBJ  = $(SRC:.c=.o)

#
# Targets
#

all:    	nondebug install 

mini_el:	$(OBJ)
	$(CC) -o $(EXEC) $(OBJ) $(CFLAGS) $(LIB_DIR) $(LIBSOCKET) $(LIBNSL)

top_debug:	debug install
top_nondebug:	nondebug install
top_sterile:	sterile

debug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(DEBUG)" 
 
nondebug:
	make -f Makefile $(EXEC) CFLAGS="$(CFLAGS) $(NONDEBUG)" 

install:	$(EXEC)
	cp $(EXEC) $(INSTALL)

clean:
	rm -f $(EXEC) $(OBJ)
 
sterile:
	rm -f $(EXEC) $(OBJ) $(INSTALL)/$(EXEC)
 
#
#  Dependencies
#
 
