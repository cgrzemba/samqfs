# $Revision: 1.11 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
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
LIB_HOME = $(DEPTH)/src/lib/samcat

include $(DEPTH)/mk/common.mk

# build 64bit lib also in 32bit userland
BUILD_64BIT = yes

SRC_VPATH = $(DEPTH)/src/catalog
vpath %.c $(SRC_VPATH)

LIB = samcat
LIB_SRC = catlib.c

DEPCFLAGS += -I$(DEPTH)/src/catalog/include
LIB_LIBS += -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut $(LIBSO) -lgen -lintl

CHECKSRC = $(LIB_SRC)
ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNOPTS += -Dlint -DTEST
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk

