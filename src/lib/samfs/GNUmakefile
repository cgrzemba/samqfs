# $Revision: 1.13 $

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
LIB_HOME = $(DEPTH)/src/lib/samfs

include $(DEPTH)/mk/common.mk

LIB = samfs
LIB_SRC = \
		catalog.c \
		csempty.c \
		cssimple.c \
		csuser.c \
		dev_log.c \
		dir_io.c \
		exit_fifo.c \
		fs_interface.c \
		gen_labels.c \
		ident_dev.c \
		misc.c \
		mount.c \
		odblank.c \
		odfile.c \
		optic.c \
		parsecmd.c \
		position.c \
		preview.c \
		scan.c \
		scsi_cmd.c \
		sef.c \
		send_todo.c   \
		table_search.c \
		tape.c \
		tape2.c \
		tapealert.c

DEPCFLAGS += -I$(OBJ_DIR) $(THRCOMP)
LIB_LIBS +=  -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamcat $(LIBSO) \
	-lgen -ldl -lthread -lposix4 -lintl -lsysevent -lnvpair

ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNLIBS =

include $(DEPTH)/mk/targets.mk

.INIT:	$(OBJ_DIR)/dev_logmsgs

$(OBJ_DIR)/cssimple.o: cssimple.c
	$(CC) -c $(CFLAGS) -xO4 -xunroll=8 -o $@ cssimple.c

$(OBJ_DIR)/dev_log.o: $(OBJ_DIR)/dev_logmsgs dev_log.c

$(OBJ_DIR)/dev_logmsgs: dev_log.msg dev_logmsg.awk
	rm -f $(OBJ_DIR)/dev_logmsgs
	/usr/bin/sed -e's/ /|/' dev_log.msg | /usr/bin/nawk -f dev_logmsg.awk >$(OBJ_DIR)/dev_logmsgs
	chmod -w $(OBJ_DIR)/dev_logmsgs

include $(DEPTH)/mk/depend.mk
