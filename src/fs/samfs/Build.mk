# $Revision: 1.22 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

SRC_VPATH = ../lib
vpath %.c $(SRC_VPATH)

ifeq ($(OS), SunOS)
# SunOS

MODULE = samfs

SRCS1 = bio.c cacl.c clcall.c client.c clmisc.c clcomm.c \
	clvnops.c cquota.c creclaim.c ctrunc.c cvnops.c getdents.c \
	iget.c init.c iput.c lqfs.c lqfs_debug.c lqfs_log.c lqfs_map.c \
	lqfs_thread.c lqfs_top.c map.c mount.c osd.c page.c psyscall.c \
	qfs_log.c qfs_trans.c rwio.c scd.c stage.c syscall.c thread.c \
	trace.c vfsops.c

SRCS2 = acl.c amld.c arfind.c balloc.c block.c create.c event.c fioctl.c \
	ialloc.c inode.c lookup.c quota.c reclaim.c remove.c rename.c \
	rmedia.c rmscall.c samscall.c san.c segment.c server.c srcomm.c \
	srmisc.c staged.c truncate.c uioctl.c update.c vnops.c

COMMON_SRCS = common_subr.c extent.c setdau.c

ifeq ($(PLATFORM), sparc)
SPARC_INLINE = ../include/sparc.il
endif

INCFLAGS = -I../include -I../include/$(OBJ_DIR) -I$(DEPTH)/include/pub/$(OBJ_DIR) -D_KERNEL -D_SYSCALL32 $(PROD_BUILD) $(SPARC_INLINE)

DEPCFLAGS += $(INCFLAGS)

CFLAGS  += $(KERNFLAGS) -xO3

LDFLAGS = -r
else
# Linux

MODULE = SUNWqfs.o

SRCS1 = clcall.c client.c clmisc.c clcomm.c \
		creclaim.c ctrunc.c map.c  psyscall.c rwio.c \
		mount.c stage.c syscall.c trace.c thread.c \
		iput.c scd.c

SRCS2 = bio_linux.c cvnops_linux.c clvnops_linux.c \
		getdents_linux.c iget_linux.c init_linux.c \
		linux_subs.c samsys_linux.c vfsops_linux.c

COMMON_SRCS = extent.c setdau.c

INCFLAGS = -include $(LINUX_INCLUDE)/linux/modversions.h \
	-I../include -I../include/$(OBJ_DIR) -I$(DEPTH)/include/pub/$(OBJ_DIR)
DEPCFLAGS += -DMODULE -DMODVERSIONS -D__KERNEL__ -D_KERNEL -DSAM_TRACE $(INCFLAGS) $(MODELFLAGS)

ifeq (x86_64,$(PROC))
CFLAGS += -mcmodel=kernel
endif
CFLAGS += -O2

LDFLAGS = -i
endif
# End ifeq ($(OS), SunOS)

MODULE_SRC = $(SRCS1) $(SRCS2) $(COMMON_SRCS)

#
# Source for 'make depend'.  Since there are 2 versions of mount.c (./mount.c ../lib/mount.c)
# we need to explicitly list the make depend source here rather than let the depend target
# generate the list from PROG_SRC and vpath.
#
DEPEND_SRC = $(SRCS1) $(SRCS2) $(addprefix $(SRC_VPATH)/, $(COMMON_SRCS))

#
# Lint source and options
#
LNSRC = $(SRCS1) $(SRCS2) $(addprefix $(SRC_VPATH)/, $(COMMON_SRCS))
LNOPTS = -nuxms -erroff=E_INCL_NUSD,E_INCL_MNUSD,E_CONSTANT_CONDITION \
	-erroff=E_STATIC_UNUSED,E_EXPR_NULL_EFFECT -Dlint
LNLIBS =

LD = ld

include $(DEPTH)/mk/targets.mk

ifeq ($(OS), SunOS)
# SunOS
install:	pre_install

pre_install:	$(OBJ_DIR)/$(MODULE) modunload
	/usr/ucb/install -m 755 -g sys -o `/usr/ucb/whoami` $(OBJ_DIR)/$(MODULE) $(FSDEST)
	# Enable init to restart fsd & notify init to restart fsd.
	chmod ugo+x /usr/lib/fs/samfs/sam-fsd
	@-/bin/kill -HUP 1
	if [ -x /sbin/bootadm ] ; then \
		/sbin/bootadm update-archive > /dev/null 2>&1; \
	fi

modload: install
	modload $(FSDEST)/$(PROG)
	sync

modunload:
	-samd stop
	# Unmount all mounted sam file systems
	- mount -p | grep samfs | cut -f 1 -d ' ' | xargs -n1 umount
	sleep 1
	- mount -p | grep samfs | cut -f 1 -d ' ' | xargs -n1 umount
	# Stop fsd and prevent init from restarting it
	chmod ugo-x /usr/lib/fs/samfs/sam-fsd
	@-/bin/ps -e | /usr/bin/grep sam | /usr/bin/grep -v grep | /usr/bin/cut -c1-6 | /usr/bin/xargs kill -TERM
	sleep 2
	- modunload -i `modinfo | grep samfs | cut -c0-4`
	- modunload -i `modinfo | grep samioc | cut -c0-4`
else
# Linux
all:
	cp $(OBJ_DIR)/$(MODULE)  $(OBJ_DIR)/$(MODULE).tmp
	./squish.pl $(OBJ_DIR)/$(MODULE).tmp $(OBJ_DIR)/$(MODULE)

install:
	if [ ! -d $(LINUX_KERNEL_SAMFS) ] ; then \
		/bin/mkdir -p $(LINUX_KERNEL_SAMFS); \
		/bin/chmod 777 $(LINUX_KERNEL_SAMFS); \
	fi
	$(INSTALL) $(OBJ_DIR)/SUNWqfs.o $(LINUX_KERNEL_SAMFS)
endif
# End ifeq ($(OS), SunOS)

include $(DEPTH)/mk/depend.mk
