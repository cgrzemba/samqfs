# $Revision: 1.18 $

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

include $(DEPTH)/mk/common.mk


PROG = sharefsd
PROG_SRC = \
	sharefs.c \
	client.c \
	configrw.c \
	sys.c \
	verify.c \
	config.c \
	configcmp.c

ifeq ($(OS), SunOS)
PROG_SRC += server.c
endif

ifeq ($(BUILD_SC_AGENT), yes)
CLUSTERINC =  -DCLUSTER_SVCS -I /usr/cluster/include
CLUSTERLD =  -ldl
endif

ifeq ($(OS), Linux)
LADM = 
else
LADM = -ladm
endif

LIBSO = $(LIBSO_OPT)/usr/lib/fs/samfs
PROG_LIBS = ../lib/$(OBJ_DIR)/libfscmd.a \
		$(STATIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) \
			-lsam -lsamut -lsamspm $(LIBSO) \
		$(DYNAMIC_OPT) -lsamconf \
		$(LADM) -lnsl -lresolv -lpthread $(CLUSTERLD)

ifeq ($(OS), SunOS)
PROG_LIBS += -lsysevent -lnvpair -lsocket
endif

INCFLAGS = -D_POSIX_PTHREAD_SEMANTICS -I. -I../include -I../include/$(OBJ_DIR) \
	-I$(INCLUDE)/pub/$(OBJ_DIR) \
	-I$(INCLUDE) $(THRCOMP) $(CLUSTERINC)
DEPCFLAGS += $(INCFLAGS) $(VERS)
CFLAGS += -g

LNOPTS = -numxs -erroff=E_STATIC_UNUSED -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
