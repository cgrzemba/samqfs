# $Revision: 1.17 $

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

#	linux.mk - definitions for a Linux build environment

MSGDEST = $(BASEDIR)/usr/share/locale/C/LC_MESSAGES

LINUX_KERNEL := $(shell uname -r)
LINUX_INCLUDE = /lib/modules/$(LINUX_KERNEL)/build/include
LINUX_CONFIG = /lib/modules/$(LINUX_KERNEL)/build/.config
LINUX_KERNEL_SAMFS = /lib/modules/$(LINUX_KERNEL)/kernel/fs/samfs

PROC = $(shell uname -m)

ATHLON=
CPU_CHECK := $(shell grep AMD /proc/cpuinfo)
ifdef CPU_CHECK
ifneq (x86_64, ${PROC})
ATHLON= _athlon
endif
endif
OS_CHECK := $(shell grep -is suse /etc/issue)
ifndef OS_CHECK
OSFLAGS= -DRHE_LINUX
export OS_TYPE=redhat${ATHLON}
else
OSFLAGS= -DSUSE_LINUX
export OS_TYPE=suse${ATHLON}
endif
OSFLAGS += -D${PROC}
ifeq (x86_64, ${PROC})
OSFLAGS += -D_ASM_X86_64_SIGCONTEXT_H -mno-red-zone
endif

OS_ARCH= $(OS)_$(OS_TYPE)_$(LINUX_KERNEL)_$(PROC)
export OS_ARCH

KERNEL_UNCLEAN=$(subst SMP,smp,$(LINUX_KERNEL))
export KERNEL_UNCLEAN
KERNEL_MAJOR= $(shell uname -r | cut -d- -f1 - | cut -d. -f2)
OSFLAGS += -DKERNEL_MAJOR=$(KERNEL_MAJOR)
KERNEL_MINOR= $(shell uname -r | cut -d- -f1 - | cut -d. -f3)
OSFLAGS += -DKERNEL_MINOR=$(KERNEL_MINOR)

LIBSO_OPT = -Wl,-R
STATIC_OPT = -Wl,-Bstatic
DYNAMIC_OPT = -Wl,-Bdynamic
SHARED_CFLAGS = -fPIC -shared
DEPCFLAGS = -I$(DEPTH)/include $(OSFLAGS) -I$(LINUX_INCLUDE)

CMDECHO = /bin/echo
CMDWHOAMI = /usr/bin/whoami
CMDMCS = echo mcs 

CC = /usr/bin/gcc
GCC = /usr/bin/gcc
LD = $(CC)
AWK = /bin/awk
PERL= $(shell which perl)
MAKEDEPEND = /usr/X11R6/bin/makedepend -Dlinux
LINT = echo lint
INSTALL = /usr/bin/install

#
# mcs definitions
#
MCSOPT = $(shell echo "-c -a '@(=)Copyright (c) `/bin/date +%Y`, Sun Microsystems, Inc.  All Rights Reserved' -a '@(=)Built `/bin/date +%D` by `$(CMDWHOAMI)` on `/bin/uname -n` `/bin/uname -s` `/bin/uname -r`.'" | /usr/bin/tr = '\043')
MCS = $(CMDMCS) $(MCSOPT)


