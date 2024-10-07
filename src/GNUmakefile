# $Revision: 1.21 $

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

DEPTH = ..

include $(DEPTH)/mk/common.mk


#
# Create OBJ_DIR to hold nl_messages.cat
#
__dummy := $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR) )


#
# Common library directory (must be built first)
#
ifneq ($(SAM_OPEN_SOURCE), yes)
LDIRS = robots/vendor_supplied lib
else
LDIRS = lib
endif


#
# Common directories for all server/client builds
#
COMMON_PDIRS = \
	cmd \
	fs \
	gnucmd \
	os \
	tools

PDIRS = $(COMMON_PDIRS)

#
# Additional SunOS only build directories
#
ifeq ($(OS), SunOS)
PDIRS += \
		conf \
		fsalog \
		drivers \
		catalog \
		rft \
		releaser \
		robots \
		sam-init \
		sampm \
		scanner \
		stageall \
		stager \
		recycler \
		utility \
		archiver \
		man \
		rpc \
		remote_sam \
		notify 

PROG = assert
PROG_SRC = assert.c
endif	# SunOS

CATMSGS = catalog.msg
CATALOG = $(OBJ_DIR)/nl_messages.cat

DIRS = $(LDIRS) $(PDIRS)

include $(DEPTH)/mk/targets.mk

all: $(OBJ_DIR)/nl_messages.cat

$(OBJ_DIR)/nl_messages.cat: catalog.msg
	rm -f             $(OBJ_DIR)/nl_messages.cat
	/usr/bin/gencat   $(OBJ_DIR)/nl_messages.cat catalog.msg
	/bin/chmod -w $(OBJ_DIR)/nl_messages.cat

ifeq ($(OS), SunOS)
install: $(OBJ_DIR)/nl_messages.cat install_more
install_more: nl_messages.cat
	$(INSTALL) -d $(MSGDEST)
	$(INSTALL) $(MSGINST) $(OBJ_DIR)/nl_messages.cat $(MSGDEST)/SUNWsamfs
	@echo "Note: Only the message catalog was installed."
	@echo "      Use pkgadd to install the rest of the files at this level."
	@echo "      All lower level install targets should work fine."
endif

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
$(PDIRS):	$(LDIRS)
lib:		robots/vendor_supplied
cmd:		fs
sam-init:	fs
remote_sam:	fs
tools:		fs
utility:	fs
samqfsui:	lib/sammgmtjni

include $(DEPTH)/mk/depend.mk
