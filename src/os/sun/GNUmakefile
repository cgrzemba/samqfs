# $Revision: 1.14 $

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

BUILD_64BIT = yes

#
# Build kernel symbol table entries for mdb's use.  This should
# only be defined in GNUmakefiles that define MODULE_SRC.
#
BUILD_STABS = yes

MODULE = samioc
MODULE_SRC = samioc.c

DEPCFLAGS += -D_KERNEL $(VERS)

CFLAGS  += $(KERNFLAGS) -xO3

LD = ld
LDFLAGS = -r

LNOPTS += -n
LNLIBS =

include $(DEPTH)/mk/targets.mk

install: all .WAIT modunload
	/usr/ucb/install -m 755 -g sam -o `/usr/ucb/whoami` $(OBJ_DIR)/samioc $(SYSDEST)
	# Enable init to restart fsd & notify init to restart fsd.
	chmod ugo+x /usr/lib/fs/samfs/sam-fsd
	@-/bin/kill -HUP 1

modunload:
	sync
	samd stop
	# Unmount all mounted sam file systems
	- mount -p | grep samfs | cut -f 1 -d ' ' | xargs -n1 umount
	sleep 1
	- mount -p | grep samfs | cut -f 1 -d ' ' | xargs -n1 umount
	# Stop fsd and prevent init from restarting it
	chmod ugo-x /usr/lib/fs/samfs/sam-fsd
	@-/bin/ps -e | /usr/bin/grep sam | /usr/bin/grep -v grep | /usr/bin/cut -c1-6 | /usr/bin/xargs kill -TERM
	sleep 2
	- modunload -i `modinfo | grep samfs | cut -c0-4`

include $(DEPTH)/mk/depend.mk
