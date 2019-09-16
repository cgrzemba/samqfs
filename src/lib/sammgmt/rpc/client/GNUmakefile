# $Revision: 1.20 $

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

DEPTH = ../../../../..
LIB_HOME = ../../../src/lib/sammgmt/rpc/client

include $(DEPTH)/mk/common.mk

DEPCFLAGS = \
	-DAPI_DEBUG=syslog \
	-DDEBUG \
        $(OSDEPCFLAGS) \
	-I$(INCLUDE) \
	-I$(INCLUDE)/sam/$(OBJ_DIR) \
	-DSAMRPC_CLIENT $(DEBUGCDEFS) $(METADATA_SERVER) $(THRCOMP)

SRC_VPATH = ../.. ../../../samut ../../../samconf ../xdr
vpath %c $(SRC_VPATH)

LIB = fsmgmtrpc
LIB_SRC = \
	sammgmt_clnt.c \
	regis_clnt.c \
	task_schedule_clnt.c \
	archive_clnt.c \
	faults_clnt.c \
	license_clnt.c \
	device_clnt.c \
	recycle_clnt.c \
	stage_clnt.c \
	load_clnt.c \
	diskvols_clnt.c \
	filesystem_clnt.c \
	mgmt_clnt.c \
	notify_summary_clnt.c \
	recyc_sh_wrap_clnt.c \
	job_history_clnt.c \
	release_clnt.c \
	hosts_clnt.c \
	arch_set_clnt.c \
	restore_clnt.c \
	file_util_clnt.c \
	report_clnt.c \
	cmd_dispatch_clnt.c

LIB_SRC += \
	log.c \
	list.c \
	util.c \
	types.c \
	memory_free.c \
	custmsg.c \
	dev_to_nm.c \
	fsizestr.c \
	intervalstr.c \
	mapfile.c \
	media.c \
	sam_syslog.c \
	setfield.c \
	strerrno.c \
	trace.c \
	error.c \
	sam_syscall.c

LIB_SRC += \
	list_xdr.c \
	types_xdr.c \
	archive_xdr.c \
	device_xdr.c \
	media_xdr.c \
	catalog_xdr.c \
	filesystem_xdr.c \
	load_xdr.c \
	recycle_xdr.c \
	stage_xdr.c \
	license_xdr.c \
	release_xdr.c \
	faults_xdr.c \
	diskvols_xdr.c \
	notify_summary_xdr.c \
	job_history_xdr.c \
	sammgmt_xdr.c \
	hosts_xdr.c \
	arch_set_xdr.c \
	restore_xdr.c \
	report_xdr.c \
	cmd_dispatch_xdr.c


LIB_LIBS += \
	-L$(DEPTH)/lib/$(OBJ_DIR) -lsysevent -lnvpair $(LIBSO) -lpthread -lsocket -lnsl -lgen -lintl

LNOPTS = -a -u -m -x -o$(LIB)
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
