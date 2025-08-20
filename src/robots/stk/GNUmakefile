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
ifeq ($(DEBUG), yes)
DBG_TARGET = top_debug
DBG_OPT=-g
else
DBG_TARGET = top_nondebug
DBG_OPT=
endif


STKABSDEST = $(shell readlink -f $(STKDEST))
ACSLIB = $(STKDEST)/libapi.so
ACSCOM = $(STKDEST)/libcl.so
ACSUTL = $(STKDEST)/libutl.so
ACSIPC = $(STKDEST)/libipc.so
# SSIXDR = $(STKDEST)/libssixdr.so $(STKDEST)/SSIXDR.a
SSIXDR = ../../lib/amd64/SSIXDR.a
ACSSSI = ssi
MINIEL = mini_el

CDK = $(shell readlink -f stk_release)
STK_ABSDIR = $(shell readlink -f $(STKDEST))

.NOTPARALLEL:

# stklibs: $(ACSCOM) $(ACSUTL) $(ACSLIB) $(SSIXDR) $(ACSIPC)

$(SSIXDR):
	-(cd stk_release/src/xdr; $(MAKE_COMMAND) DEBUG="$(DBG_OPT)" $(DBG_TARGET) PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR))

$(ACSSSI): $(SSIXDR)
	-(cd stk_release/src/ssi; env; $(MAKE_COMMAND) INSTALL="../../bin/amd64"			\
		SYSINST="$(SYSINST)" STKDEST="$(DESTDIR)"				\
		LIBSO="$(subst lib,lib/stk,$(LIBSO))" CDK=$(shell pwd)/stk_release CDK_BUILD_OPTION=BASE PLATFORM=$(PLATFORM) DEBUG="$(DBG_OPT)" $(DBG_TARGET))

$(MINIEL):
	-(cd stk_release/src/mini_el; $(MAKE_COMMAND) INSTALL="../../bin/amd64"			\
		SYSINST="$(SYSINST)" STKDEST="$(DESTDIR)"				\
		LIBSO="$(subst lib,lib/stk,$(LIBSO))" CDK=$(shell pwd)/stk_release CDK_BUILD_OPTION=BASE PLATFORM=$(PLATFORM) DEBUG="$(DBG_OPT)" $(DBG_TARGET))

$(ACSCOM):
	-(cd stk_release/src/common_lib; env; $(MAKE_COMMAND) DEBUG="$(DBG_OPT)" $(DBG_TARGET) PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR))

$(ACSUTL):
	(cd stk_release/src/acsutl;  $(MAKE_COMMAND) DEBUG="$(DBG_OPT)" $(DBG_TARGET) PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR))

$(ACSIPC):
	-(cd stk_release/src/acsipc; $(MAKE_COMMAND) DEBUG="$(DBG_OPT)" $(DBG_TARGET) PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR))

$(ACSLIB):
	-(cd stk_release/src/acsapi; $(MAKE_COMMAND) DEBUG="$(DBG_OPT)" $(DBG_TARGET) PLATFORM=$(PLATFORM) CDK=$(CDK) CDK_BUILD_OPTION=BASE LIB_DIR=$(STK_ABSDIR))

# stkssi: stklibs
ACSLIBS = $(ACSCOM) $(ACSUTL) $(ACSIPC) $(ACSLIB)
$(OBJ_DIR)/stk_helper: $(ACSLIBS)

clean_stk:
	rm -f $(ACSLIBS)
	(cd stk_release/src/acsapi; env; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	(cd stk_release/src/common_lib; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	(cd stk_release/src/acsutl; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	(cd stk_release/src/acsipc; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	(cd stk_release/src/xdr; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))
	(cd stk_release/src/ssi; $(MAKE_COMMAND) clean CDK=$(CDK) PLATFORM=$(PLATFORM) LIB_DIR=$(STK_ABSDIR))

install:	install_more
install_more:
	$(INSTALL) -d $(LIBDEST)/stk
	rm -f $(LIBDEST)/stk/* $(ADMDEST)/ssi_so
	cp $(DEPTH)/lib/$(OBJ_DIR)/libapi.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libcl.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libipc.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/libutl.so $(LIBDEST)/stk
#	cp $(DEPTH)/lib/$(OBJ_DIR)/libssixdr.so $(LIBDEST)/stk
	cp $(DEPTH)/lib/$(OBJ_DIR)/ssi_so $(ADMDEST)
	$(INSTALL) $(SYSINST) $(WHOAMI) $(ADMDEST)/sam-$(WHOAMI)d
	$(INSTALL) $(SYSINST) $(HELPER) $(ADMDEST)
	$(INSTALL) $(SYSINST) $(SSISH) $(ADMDEST)

OS_RELEASE_VER = $(shell  echo $(OS_RELEASE) | tr -d -c 0-9).1
DBG_SUFF.DBG = _DEBUG
DBG_SUFF= $(DBG_SUFF.$(DEBUG:yes=DBG))
pkg:    $(PROG) $(ACSLIBS) $(ACSSSI) $(MPROGS) $(MINIEL)
	pkgmogrify -D OSRELEASE=$(OS_RELEASE_VER) -D SAMQFS_VERSION=$(SAMQFS_VERSION) stk.p5m > stk.p5m.mog
	pkglint stk.p5m.mog
	pkgsend publish -d $(OBJ_DIR) -d. -s ../../../repo/i386-$(OS_RELEASE)/ stk.p5m.mog && rm stk.p5m.mog
	pkgmogrify -D OSRELEASE=$(OS_RELEASE_VER) acsls-client-toolkit.p5m > acsls-client-toolkit.p5m.mog
	pkglint acsls-client-toolkit.p5m.mog
	pkgsend publish -d ./stk_release -d $(STKDEST) -d . -s ../../../repo/i386-$(OS_RELEASE)/ acsls-client-toolkit.p5m.mog && rm acsls-client-toolkit.p5m.mog

include $(DEPTH)/mk/depend.mk

show:
	@echo "obj_dir: $(OBJ_DIR)/stk_helper"
	@echo "acls_libs: $(ACSLIBS)"
	@echo "VERSION: $(SAMQFS_VERSION)"
	@echo "DEBUG: $(DEBUG)"
	@echo "INSTALL: $(INSTALL)"
	@echo "STKDEST: $(STKDEST)"
	@echo "CDK: $(CDK)"
