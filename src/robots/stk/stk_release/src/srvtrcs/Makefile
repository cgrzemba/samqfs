# SccsId @(#)Makefile            7.2 05/03/11 
#                                                               
# Copyright (c) 2011, 2011, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      Makefile                                      
#                                                               
# Description:    srvtrcs module makefile                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720/CDK240  Joseph Nofi     05/03/11                       
#     Created for CDK to add XAPI support.
#                                                               
#===============================================================
#
# Include the compiler definitions needed to build
# on your specific host.
#
include ../../etc/make.ini

#
# Definitions
#
H       =../h
INSTALL =../../bin
 
#
# Flags/Options
# 
CC     = $(COMPILER)
CFLAGS = $(COMMON_CC_FLAGS) -I$(H)
       
#
# Libraries
# 
EXEC  = srvtrcs
SRC   = srvtrcs.c
OBJ   = $(SRC:.c=.o)

#
# Targets
#
all:	nondebug install 

srvtrcs:	$(OBJ)
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
	rm -f $(OBJ) $(EXEC) $(INSTALL)/$(EXEC)

#
# Dependencies
#

