# $Revision: 1.3 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..
OBJ_DEPTH = ../../..
LIB_HOME = $(DEPTH)/src/lib/pax_hdr

DEPCFLAGS += -I$(OBJ_DIR)

include $(DEPTH)/mk/common.mk

LIB = pax_hdr
LIB_SRC = \
	pax_util.c \
	pax_pair.c \
	pax_xhdr.c \
	pax_hdr.c

LIB_LIBS +=  -L$(DEPTH)/lib/$(OBJ_DIR) -lsamut $(LIBSO)

ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNLIBS =

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(USERINST) $(OBJ_DIR)/lib$(LIB).so $(LIBDEST)

test : $(OBJ_DIR)/libpax_hdr.so $(PROG)
	$(OBJ_DIR)/$(PROG)

include $(DEPTH)/mk/depend.mk
