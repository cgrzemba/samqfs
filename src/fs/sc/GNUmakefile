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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

MPROGS = \
	scqfs_init \
	scqfs_start \
	scqfs_prenet_start \
	scqfs_stop \
	scqfs_postnet_stop \
	scqfs_monitor_start \
	scqfs_monitor_stop \
	scqfs_monitor_check \
	scqfs_probe \
	scqfs_validate \
	scqfs_update \
	scqfs_fini

COMMON_SRC = scqfs_common.c

scqfs_init_SRC = scqfs_init.c $(COMMON_SRC)
scqfs_start_SRC = scqfs_start.c $(COMMON_SRC)
scqfs_prenet_start_SRC = scqfs_prenet_start.c $(COMMON_SRC)
scqfs_stop_SRC = scqfs_stop.c $(COMMON_SRC)
scqfs_postnet_stop_SRC = scqfs_postnet_stop.c $(COMMON_SRC)
scqfs_monitor_start_SRC = scqfs_monitor_start.c $(COMMON_SRC)
scqfs_monitor_stop_SRC = scqfs_monitor_stop.c $(COMMON_SRC)
scqfs_monitor_check_SRC = scqfs_monitor_check.c $(COMMON_SRC)
scqfs_probe_SRC = scqfs_probe.c $(COMMON_SRC)
scqfs_validate_SRC = scqfs_validate.c $(COMMON_SRC)
scqfs_update_SRC = scqfs_update.c $(COMMON_SRC)
scqfs_fini_SRC = scqfs_fini.c  $(COMMON_SRC)

LIBSO = -R/usr/cluster/lib:/opt/SUNWsamfs/lib

COMMON_OBJS = $($(COMMON_SRC):%.c=$(OBJ_DIR)/%.o)
COMMON_LIBS = $(LIBSO) -L$(SC31TOP)usr/cluster/lib -L$(DEPTH)/lib/$(OBJ_DIR) -L../lib/$(OBJ_DIR) \
	-lsocket -lnsl -ldsdev -lscha -lsamut $(STATIC_OPT) -lfscmd \
	$(DYNAMIC_OPT) -lsam -ladm -ldl


LDFLAGS = $(COMMON_OBJS)

# I18N Stuff
TEXT_DOMAIN = SUNW_SC_QFS
MESSAGE_DIR = /opt/SUNWsamfs/lib/locale

#
# TOP refers to the place where the SUNWscdev package is
# installed, typically on the build machine itself, in
# /usr/cluster/
#
SC31TOP = /

DEPCFLAGS = -D TEXT_DOMAIN=\"$(TEXT_DOMAIN)\"
DEPCFLAGS += -D MESSAGE_DIR=\"$(MESSAGE_DIR)\"
DEPCFLAGS += -I$(SC31TOP)usr/cluster/include
DEPCFLAGS += -I$(DEPTH)/include
DEPCFLAGS += -I$(DEPTH)/src/fs/include -I$(DEPTH)/src/fs/include/$(OBJ_DIR)

CFLAGS += -g
CFLAGS += -xildoff

LNFLAGS = -I/ws/sc31-tools/ref_proto/s9_58shwpl3/root_sparc/usr/include
LNFLAGS += -I$(SC31TOP)usr/cluster/include
LNFLAGS += -I$(DEPTH)/include
LNFLAGS += -I$(DEPTH)/src/fs/include
LNFLAGS += -Nlevel=3 -m -u

SCDEVDIRS = /usr/cluster/include /usr/cluster/lib
SPEC_OBJ = ../lib/$(OBJ_DIR)/setsyscall.o

scqfs_init_LIBS = $(SPEC_OBJ) $(COMMON_LIBS)
scqfs_start_LIBS = $(COMMON_LIBS)
scqfs_prenet_start_LIBS = $(SPEC_OBJ) $(COMMON_LIBS)
scqfs_stop_LIBS = $(COMMON_LIBS)
scqfs_postnet_stop_LIBS = $(COMMON_LIBS)
scqfs_monitor_start_LIBS = $(COMMON_LIBS)
scqfs_monitor_stop_LIBS = $(COMMON_LIBS)
scqfs_monitor_check_LIBS = $(COMMON_LIBS)
scqfs_probe_LIBS = $(COMMON_LIBS)
scqfs_validate_LIBS = $(COMMON_LIBS)
scqfs_update_LIBS = $(COMMON_LIBS)
scqfs_fini_LIBS = $(COMMON_LIBS)

include $(DEPTH)/mk/targets.mk

.INIT: $(SCDEVDIRS) $(COMMON_OBJS)

$(SCDEVDIRS):
	@if [ ! -d $@ ] ; then \
		echo "The SUNWscdev package must be installed" ; \
		echo "in order to build the SunCluster agent" ; \
		exit 1 ; \
	fi

include $(DEPTH)/mk/depend.mk
