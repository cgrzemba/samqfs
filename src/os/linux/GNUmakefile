# $Revision: 1.15 $

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

ifeq (4,$(KERNEL_MAJOR))

ifeq (x86_64,$(PROC))
MODELFLAGS= -mcmodel=kernel
endif

SAM_INC = ../fs/include
INCFLAGS = $(DEPCFLAGS) -I$(SAM_INC)

INCFLAGS += -include $(LINUX_INCLUDE)/linux/modversions.h

#CFLAGS = -DFAST_ABSTRACTION -DMODULE -DMODVERSIONS -D__KERNEL__ $(INCFLAGS) $(MODELFLAGS) $(CERRWARN)
CFLAGS = -DMODULE -DMODVERSIONS -D__KERNEL__ $(INCFLAGS) $(MODELFLAGS) $(CERRWARN)

SAMGPL_SRCS = samgpl.c rfs_misc.c
ifeq ($(PROC),i686)
SAMGPL_SRCS += libgcc2.c
endif
SAMGPL_OBJS = $(SAMGPL_SRCS:%.c=$(OBJ_DIR)/%.o)

QFSKI_SRCS= qfs_ki.c
QFSKI_OBJS= $(QFSKI_SRCS:%.c=$(OBJ_DIR)/%.o)

DEPEND_SRC = $(SAMGPL_SRCS) $(QFSKI_SRCS)

MODULE = SUNWqfs_ki.o
MODULE_SRC = $(QFSKI_SRCS)

LD = ld
LDFLAGS = -i

include $(DEPTH)/mk/targets.mk

all: $(OBJ_DIR)/SUNWqfs_open.o

$(OBJ_DIR)/SUNWqfs_open.o: $(SAMGPL_OBJS)
	$(LD) -i -o $@ $(SAMGPL_OBJS)

install:
	if [ ! -d $(LINUX_KERNEL_SAMFS) ] ; then \
		/bin/mkdir -p $(LINUX_KERNEL_SAMFS); \
		/bin/chmod 777 $(LINUX_KERNEL_SAMFS); \
	fi
	$(INSTALL) $(OBJ_DIR)/SUNWqfs_open.o $(LINUX_KERNEL_SAMFS)
	$(INSTALL) $(OBJ_DIR)/SUNWqfs_ki.o $(LINUX_KERNEL_SAMFS)

include $(DEPTH)/mk/depend.mk

else
# kernel 2.6
all:
	@DEBUG=$(DEBUG) OSFLAGS="$(OSFLAGS)" $(MAKE) -f Makefile.kbuild

clean:
	@DEBUG=$(DEBUG) OSFLAGS="$(OSFLAGS)" $(MAKE) -f Makefile.kbuild clean

clobber:
	@DEBUG=$(DEBUG) OSFLAGS="$(OSFLAGS)" $(MAKE) -f Makefile.kbuild clean

clobberall:
	-rm -rf $(OBJ_BASE)

endif
