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

SRC_VPATH = ../common
vpath %.c $(SRC_VPATH)

PROG = stk
PROG_SRC = \
	clear.c \
	init.c \
	main.c \
	man_work.c \
	media_move.c \
	message.c \
	stk_drive.c \
	stk_misc.c \
	transport.c \
	work_cmds1.c

#
# Additional source files from ../common
#
PROG_SRC += \
	misc.c \
	down.c \
	drive.c \
	drive1.c \
	work_cmds.c \
	chk_drives.c \
	common_scsi.c

DEPCFLAGS += -Istk_includes $(THRCOMP)

PROG_LIBS = $(STATIC_OPT) -L ../lib/$(OBJ_DIR) -lrobots \
	$(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) \
	-lsamfs -lsamcat -lsamut $(LIBSO) $(THRLIBS) -lgen

STK_INCLUDES = stk_release/src/h

ACSLS_CSC_TK_LIBS = -lapi -lutl -lcl -lipc
STKDEST = $(DEPTH)/lib/$(OBJ_DIR)
MPROGS = stk_helper
stk_helper_SRC = stk_helper.c
stk_helper_LIBS = $(subst lib,lib/stk,$(LIBSO)) -Bdynamic -L $(STKDEST) $(ACSLS_CSC_TK_LIBS) $(PROG_LIBS)

SSISH   = ssi.sh

LNOPTS = -a -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamfs -lsamcat -lsamut

include $(DEPTH)/mk/targets.mk

#
# The stk libraries need to be put in the pkg/vendor/stk directory
# when all is said and done.  They are built and installed into
# /opt/SUNWsamfs/lib/stk in this makefile.  The ssi_so binary needs to be
# put into pkg/vendor/srk/bin directory.  It is built and installed into
# /etc/fs/samfs in this makefile.  Neither the stk libraries not the ssi_so
# binary are built during normal build and install.

PLATFORM_sparcv9 = sparc64
PLATFORM_amd64 = x64
# PLATFORM = $(PLATFORM_$(shell isainfo -k))
# need 32bit libs
PLATFORM = i386
ifeq ($(AMD64), yes)
  PLATFORM = x64
endif

STKABSDEST = $(shell readlink -f $(STKDEST))
ACSLIB = $(STKDEST)/libapi.so
ACSCOM = $(STKDEST)/libcl.so
ACSUTL = $(STKDEST)/libutl.so
ACSIPC = $(STKDEST)/libipc.so
SSIXDR = $(STKDEST)/libssixdr.so
ACSSSI  = ssi

CDK = $(shell readlink -f stk_release)
STK_ABSDIR = $(shell readlink -f $(STKDEST))

.NOTPARALLEL:

# stklibs: $(ACSCOM) $(ACSUTL) $(ACSLIB) $(SSIXDR) $(ACSIPC)

$(ACSSSI):
	-(cd stk_release/src/ssi; $(MAKE_COMMAND) INSTALL="$(INSTALL)"			\
		SYSINST="$(SYSINST)" STKDEST="$(DESTDIR)"				\
		LIBSO="$(subst lib,lib/stk,$(LIBSO))" CDK=$(shell pwd)/stk_release CDK_BUILD_OPTION=BASE PLATFORM=$(PLATFORM) top_nondebug )

$(ACSCOM):
	-(cd stk_release/src/common_lib; $(MAKE_COMMAND) clean top_nondebug PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR) )

$(SSIXDR):
	-(cd stk_release/src/ssixdr; $(MAKE_COMMAND) clean top_nondebug PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR) )

$(ACSUTL):
	-(cd stk_release/src/acsutl; $(MAKE_COMMAND) clean top_nondebug PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR) )

$(ACSIPC):
	-(cd stk_release/src/acsipc; $(MAKE_COMMAND) clean top_nondebug PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR) )

$(ACSLIB):
	-(cd stk_release/src/acsapi; $(MAKE_COMMAND) clean top_nondebug PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR) )

# stkssi: stklibs
ACSLIBS = $(ACSCOM) $(ACSUTL) $(ACSIPC) $(ACSLIB)
$(OBJ_DIR)/stk_helper: $(ACSLIBS)

clean_stk:
	@(cd stk_release/src/acsapi; $(MAKE_COMMAND) CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR) clean)
	@(cd stk_release/src/common_lib; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	@(cd stk_release/src/acsutl; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	@(cd stk_release/src/acsipc; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	@(cd stk_release/src/xdr; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	@(cd stk_release/src/ssi; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))

install:	install_more
install_more:
	$(INSTALL) -d $(LIBDEST)/stk
	rm -f $(LIBDEST)/stk/* $(ADMDEST)/ssi_so
	cp $(DEPTH)/lib/$(OBJ_DIR)/libapi.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libcl.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libipc.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libutl.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libssixdr.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/ssi_so $(ADMDEST)
	$(INSTALL) $(SYSINST) $(WHOAMI) $(ADMDEST)/sam-$(WHOAMI)d
	$(INSTALL) $(SYSINST) $(HELPER) $(ADMDEST)
	$(INSTALL) $(SYSINST) $(SSISH) $(ADMDEST)

OS_RELEASE_VER = $(shell  echo $(OS_RELEASE) | tr -d -c 0-9).0
DBG_SUFF.DBG = _DEBUG
DBG_SUFF= $(DBG_SUFF.$(DEBUG:yes=DBG))
pkg:    $(PROG) $(ACSLIBS) $(MPROGS)
	cat stk.p5m.in | sed -e s/{{SAMQFS_VERSION}}/$(SAMQFS_VERSION)/ \
		-e s/{{OSRELEASE}}/$(OS_RELEASE_VER)/ \
		-e s/{{OS_RELEASE}}/$(OS_RELEASE)/ \
		-e s/{{DBG}}/$(DBG_SUFF)/ \
		> stk.p5m
	pkglint stk.p5m
	pkgsend publish -d. -s ../../../repo/i386-$(OS_RELEASE)/ stk.p5m

include $(DEPTH)/mk/depend.mk

show:
	@echo "obj_dir: $(OBJ_DIR)/stk_helper"
	@echo "acls_libs: $(ACSLIBS)"
	@echo "VERSION: $(SAMQFS_VERSION)"
	@echo "DEBUG: $(DEBUG)"
