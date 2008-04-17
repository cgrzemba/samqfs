# $Revision: 1.19 $

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

DEPTH = ../../../../..

include $(DEPTH)/mk/common.mk

SRC_VPATH = . ../xdr
vpath %.c $(SRC_VPATH)

MGMT_INCLUDE = $(DEPTH)/include/mgmt

DEPCFLAGS = \
	-DAPI_DEBUG=syslog \
	-DDEBUG \
	-I$(INCLUDE) \
	-DSAMRPC_SERVER $(DEBUGCDEFS) $(METADATA_SERVER) $(THRCOMP)

PROG = fsmgmtd
PROG_SRC = \
	task_schedule_svr.c \
	regis_svr.c \
	release_svr.c \
	archive_svr.c \
	faults_svr.c \
	license_svr.c \
	recycle_svr.c \
	stage_svr.c \
	load_svr.c \
	diskvols_svr.c \
	device_svr.c \
	filesystem_svr.c \
	mgmt_svr.c \
	notify_summary_svr.c \
	recyc_sh_wrap_svr.c \
	job_history_svr.c \
	hosts_svr.c \
	secure_cfg.c \
	sammgmt_svc.c \
	arch_set_svr.c \
	restore_svr.c \
	file_util_svr.c \
	report_svr.c

PROG_SRC += \
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
	hosts_xdr.c \
	sammgmt_xdr.c \
	arch_set_xdr.c \
	restore_xdr.c \
	report_xdr.c


PROG_LIBS = \
	-L$(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamapi -lgen -lfsmgmt -lnsl -lintl -ldl -lcsn -lcurl -lssl -lcrypto -lsocket $(LIBSO)


MPROGS = clientmgmt
clientmgmt_SRC = secure_cfg.c clientmgmt.c
clientmgmt_LIBS = $(PROG_LIBS) -lsocket

LNOPTS = -a -u -m -x -o$(LIB)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
