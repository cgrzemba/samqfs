# $Revision: 1.16 $

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

DEPTH = ../../../../..

DEPCFLAGS = \
	$(DEBUGCDEFS) \
	-DAPI_DEBUG=syslog \
	-I$(INCLUDE) \
	$(METADATA_SERVER) $(THRCOMP)

include $(DEPTH)/mk/common.mk

LIB = xdr
LIB_SRC = \
	archive_xdr.c \
	types_xdr.c \
	device_xdr.c \
	catalog_xdr.c \
	media_xdr.c \
	filesystem_xdr.c \
	license_xdr.c \
	load_xdr.c \
	recycle_xdr.c \
	release_xdr.c \
	stage_xdr.c \
	list_xdr.c \
	faults_xdr.c \
	diskvols_xdr.c \
	notify_summary_xdr.c \
	job_history_xdr.c \
	restore_xdr.c \
	report_xdr.c \
	sammgmt_xdr.c \
	cmd_dispatch_xdr.c

DATAFILES = $(LIB_SRC) Makefile.inst

LNOPTS = -a -u -m -x -Dlint -o$(LIB)
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
