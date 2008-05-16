# $Revision: 1.13 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..
LIB_HOME = $(DEPTH)/src/lib/samspm

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

ifeq ($(OS), SunOS)
MLIBS = samspm samspmd
samspm_SRC = spm.c
samspmd_SRC = spmd.c

LIB_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamconf $(LIBSO) -lsocket -lnsl -lgen -lintl
samspm_LIBS = $(LIB_LIBS)
samspmd_LIBS = $(LIB_LIBS)

MPROGS = spmQuery spmTestClient spmTestServerP spmTestServerS
spmQuery_SRC = spmQuery.c
spmTestClient_SRC 	= spmTestClient.c
spmTestServerP_SRC 	= spmTestServerP.c
spmTestServerS_SRC 	= spmTestServerS.c

PROG_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) \
	$(STATIC_OPT) -L $(OBJ_DIR) -lsamspmd -lsamspm -lsamconf $(DYNAMIC_OPT) $(LIBSO) \
	-lpthread -lsocket -lnsl 
spmQuery_LIBS 		= $(PROG_LIBS)
spmTestClient_LIBS 	= $(PROG_LIBS)
spmTestServerP_LIBS = $(PROG_LIBS)
spmTestServerS_LIBS = $(PROG_LIBS)
else
DEPCFLAGS += $(THRCOMP)
LIB = samspm
LIB_SRC = spm.c
LIB_LIBS = $(LIBSO) -lnsl
endif

DEPCFLAGS += $(THRCOMP)
ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNOPTS += -Dlint
LNLIBS =

include $(DEPTH)/mk/targets.mk

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
