# $Revision: 1.19 $

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
LIB_HOME = $(DEPTH)/src/lib/samut

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

LIB = samut

ifeq ($(OS), SunOS)
LIB_SRC = \
	archiver.c \
	custmsg.c \
	data.c \
	dbfile.c \
	dev_to_nm.c \
	dev_usage.c \
	disk_show.c \
	diskvols.c \
	error.c \
	filesys.c \
	format.c \
	fsd.c \
	fsizestr.c \
	getugname.c \
	id2path.c \
	intervalstr.c \
	lockout.c \
	malloc_wait.c \
	mapfile.c \
	mcfbin.c \
	media.c \
	media_to_dev.c \
	nm_to_dev.c \
	percent.c \
	readcfg.c \
	sam_malloc.c \
	sam_syslog.c \
	sblk_show.c \
	setfield.c \
	shm.c \
	signals.c \
	stager.c \
	strerrno.c \
	time_string.c \
	tp_rearchive.c \
	trace.c \
	trap.c \
	udscom.c \
	util.c \
	volidstr.c

LIB_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamconf -lsamrft $(DB_LIB) \
	-lsysevent -lnvpair $(LIBSO) -lsocket -lnsl -lgen -lintl
else

LIB_SRC = \
	custmsg.c \
	data.c \
	dev_to_nm.c \
	diskvols.c \
	error.c \
	filesys.c \
	fsizestr.c \
	intervalstr.c \
	malloc_wait.c \
	mapfile.c \
	mcfbin.c \
	media.c \
	media_to_dev.c \
	nm_to_dev.c \
	readcfg.c \
	sam_malloc.c \
	sam_syslog.c \
	setfield.c \
	strerrno.c \
	time_string.c \
	trace.c \
	udscom.c \
	util.c \
	lockout.c

LIB_LIBS = -lnsl $(LIBSO)

endif

DEPCFLAGS += -I$(INCLUDE)/sam/$(OBJ_DIR) -I$(DB_INCLUDE) $(THRCOMP)

ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(USERINST) $(OBJ_DIR)/lib$(LIB).so $(LIBDEST)

include $(DEPTH)/mk/depend.mk
