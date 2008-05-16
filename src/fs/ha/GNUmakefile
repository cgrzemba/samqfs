# $Revision: 1.7 $

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

include $(DEPTH)/mk/common.mk

MPROGS = \
	hasam_init \
	hasam_start \
	hasam_prenet_start \
	hasam_stop \
	hasam_postnet_stop \
	hasam_monitor_start \
	hasam_monitor_stop \
	hasam_monitor_check \
	hasam_probe \
	hasam_validate \
	hasam_update \
	hasam_fini

COMMON_SRC = hasam_common.c hasam_tape_common.c

hasam_init_SRC = hasam_init.c $(COMMON_SRC)
hasam_start_SRC = hasam_start.c $(COMMON_SRC)
hasam_prenet_start_SRC = hasam_prenet_start.c $(COMMON_SRC)
hasam_stop_SRC = hasam_stop.c $(COMMON_SRC)
hasam_postnet_stop_SRC = hasam_postnet_stop.c $(COMMON_SRC)
hasam_monitor_start_SRC = hasam_monitor_start.c $(COMMON_SRC)
hasam_monitor_stop_SRC = hasam_monitor_stop.c $(COMMON_SRC)
hasam_monitor_check_SRC = hasam_monitor_check.c $(COMMON_SRC)
hasam_probe_SRC = hasam_probe.c $(COMMON_SRC)
hasam_validate_SRC = hasam_validate.c $(COMMON_SRC)
hasam_update_SRC = hasam_update.c $(COMMON_SRC)
hasam_fini_SRC = hasam_fini.c $(COMMON_SRC)

LIBSO = -R/usr/cluster/lib:/opt/SUNWsamfs/lib

COMMON_OBJS = $($(COMMON_SRC):%.c=$(OBJ_DIR)/%.o)
COMMON_LIBS = $(LIBSO) -L$(SC31TOP)usr/cluster/lib \
					-L$(DEPTH)/lib/$(OBJ_DIR) -L../lib/$(OBJ_DIR) \
				-lsocket -lnsl -ldsdev -lscha -lsamut \
				$(STATIC_OPT) -lfscmd \
				$(DYNAMIC_OPT) -lsam  -lfsmgmt -lcsn \
				${STATIC_OPT} -lcurl -lssl -lcrypto \
				${DYNAMIC_OPT} -ldl


LDFLAGS = $(COMMON_OBJS)

# I18N Stuff
TEXT_DOMAIN = SUNW_HA_SAM
MESSAGE_DIR = /opt/SUNWsamfs/lib/locale

#
# TOP refers to the place where the SUNWscdev package is
# installed, typically on the build machine itself, in
# /usr/cluster/
#
SC31TOP = /

DEPCFLAGS = -D TEXT_DOMAIN=\"$(TEXT_DOMAIN)\"
DEPCFLAGS += -D MESSAGE_DIR=\"$(MESSAGE_DIR)\"
DEPCFLAGS += -I/usr/include
DEPCFLAGS += -I$(SC31TOP)usr/cluster/include
DEPCFLAGS += -I$(DEPTH)/include
DEPCFLAGS += -I$(DEPTH)/src/fs/include -I$(DEPTH)/src/fs/include/$(OBJ_DIR)

CFLAGS += -g
CFLAGS += -xildoff

LNFLAGS = -I/ws/sc31-tools/ref_proto/s9_58shwpl3/root_sparc/usr/include
LNFLAGS += -I$(SC31TOP)usr/cluster/include
LNFLAGS += -I/usr/include
LNFLAGS += -I$(DEPTH)/include
LNFLAGS += -I$(DEPTH)/src/fs/include
LNFLAGS += -Nlevel=4 -m -u

SCDEVDIRS = /usr/include /usr/cluster/include /usr/cluster/lib
SPEC_OBJ = ../lib/$(OBJ_DIR)/setsyscall.o

hasam_init_LIBS = $(SPEC_OBJ) $(COMMON_LIBS)
hasam_start_LIBS = $(COMMON_LIBS)
hasam_prenet_start_LIBS = $(SPEC_OBJ) $(COMMON_LIBS)
hasam_stop_LIBS = $(COMMON_LIBS)
hasam_postnet_stop_LIBS = $(COMMON_LIBS)
hasam_monitor_start_LIBS = $(COMMON_LIBS)
hasam_monitor_stop_LIBS = $(COMMON_LIBS)
hasam_monitor_check_LIBS = $(COMMON_LIBS)
hasam_probe_LIBS = $(COMMON_LIBS)
hasam_validate_LIBS = $(COMMON_LIBS)
hasam_update_LIBS = $(COMMON_LIBS)
hasam_fini_LIBS = $(COMMON_LIBS)

include $(DEPTH)/mk/targets.mk

.INIT: $(SCDEVDIRS) $(COMMON_OBJS)

$(SCDEVDIRS):
	@if [ ! -d $@ ] ; then \
		echo "The SUNWscdev package must be installed" ; \
		echo "in order to build the SunCluster agent" ; \
		exit 1 ; \
	fi

include $(DEPTH)/mk/depend.mk
