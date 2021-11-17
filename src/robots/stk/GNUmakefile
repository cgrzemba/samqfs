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

MPROGS = stk_helper
stk_helper_SRC = stk_helper.c
stk_helper_LIBS = $(LIBSO)/stk -Bdynamic -L $(DEPTH)/lib/$(OBJ_DIR) -lapi -lutl -lcl -lipc $(PROG_LIBS)

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

ACSLIB = libapi.so
ACSCOM = libcl.so
ACSUTL = libutl.so
ACSIPC = libipc.so
SSIXDR = libssixdr.so
ACSLIBS = $(ACSLIB) $(ACSCOM) $(ACSUTL) $(ACSIPC)
ACSSSI  = ssi

.NOTPARALLEL:

stklibs: $(ACSIPC) $(ACSCOM) $(ACSUTL) $(ACSLIB) $(SSIXDR)

stkssi: stklibs $(ACSSSI)

$(ACSSSI):
	-(cd stk_release/src/ssi; make INSTALL="$(INSTALL)"			\
		SYSINST="$(SYSINST)" STKDEST="$(DESTDIR)"				\
		LIBSO="$(LIBSO)/stk" CDK=$(shell pwd)/stk_release CDK_BUILD_OPTION=BASE PLATFORM=$(PLATFORM) top_nondebug )

$(ACSLIB):
	-(cd stk_release/src/acsapi; make INSTALL="$(INSTALL)"		\
		SYSINST="$(SYSINST)" LIBDEST="$(LIBDEST)/stk"  top_nondebug )
	$(INSTALL) -d $(DEPTH)/lib/$(OBJ_DIR)/stk/
	@if [ ! -h $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSLIB) ] ; then				\
		ln -s $(LIBDEST)/stk/$(ACSLIB) $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSLIB) ; \
	fi

$(ACSCOM):
	-(cd stk_release/src/common_lib; make INSTALL="$(INSTALL)"	\
		SYSINST="$(SYSINST)" LIBDEST="$(LIBDEST)/stk"  top_nondebug )
	$(INSTALL) -d $(DEPTH)/lib/$(OBJ_DIR)/stk/
	@if [ ! -h $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSCOM) ] ; then				\
		ln -s $(LIBDEST)/stk/$(ACSCOM) $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSCOM) ; \
	fi

$(SSIXDR):
	-(cd stk_release/src/xdr; make INSTALL="$(INSTALL)"			\
		SYSINST="$(SYSINST)" LIBDEST="$(LIBDEST)/stk"  top_nondebug )
	$(INSTALL) -d $(DEPTH)/lib/$(OBJ_DIR)/stk/
	@if [ ! -h $(DEPTH)/lib/$(OBJ_DIR)/stk/$(SSIXDR) ] ; then				\
		ln -s $(LIBDEST)/stk/$(SSIXDR) $(DEPTH)/lib/$(OBJ_DIR)/stk/$(SSIXDR) ; \
	fi

$(ACSUTL):
	-(cd stk_release/src/acsutl; make INSTALL="$(INSTALL)"		\
		SYSINST="$(SYSINST)" LIBDEST="$(LIBDEST)/stk"  top_nondebug )
	$(INSTALL) -d $(DEPTH)/lib/$(OBJ_DIR)/stk/
	@if [ ! -h $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSUTL) ] ; then				\
		ln -s $(LIBDEST)/stk/$(ACSUTL) $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSUTL) ; \
	fi

$(ACSIPC):
	-(cd stk_release/src/acsipc; make INSTALL="$(INSTALL)"		\
		SYSINST="$(SYSINST)" LIBDEST="$(LIBDEST)/stk"  top_nondebug )
	$(INSTALL) -d $(DEPTH)/lib/$(OBJ_DIR)/stk/
	@if [ ! -h $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSIPC) ] ; then				\
		ln -s $(LIBDEST)/stk/$(ACSIPC) $(DEPTH)/lib/$(OBJ_DIR)/stk/$(ACSIPC) ; \
	fi

clean_stk:
	@(cd stk_release/src/acsapi; make clean)
	@(cd stk_release/src/common_lib; make clean)
	@(cd stk_release/src/acsutl; make clean)
	@(cd stk_release/src/acsipc; make clean)
	@(cd stk_release/src/xdr; make clean)
	@(cd stk_release/src/ssi; make clean)

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

include $(DEPTH)/mk/depend.mk
