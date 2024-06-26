# $Revision: 1.45 $
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
LIB_HOME = $(DEPTH)/src/lib/sammgmt

include $(DEPTH)/mk/common.mk

SRC_VPATH = job archiver fs mcf device recycler releaser stager monitor reports \
	diskvols notification faults recyc_sh_wrap restore rpc/xdr \
	$(DEPTH)/src/fs/lib
ifndef NO_BUILD_CNS
SRC_VPATH += csn
endif
vpath %c $(SRC_VPATH)

LIB = fsmgmt
LIB_SRC = \
	sam_file_util.c \
	explorer.c \
	tail.c \
	server_info.c \
	log_info.c \
	process_job.c \
	job_control.c \
	cmd_dispatch.c \
	shared_fscmd.c \
	hosts.c \
	shared_fs_ops.c \
	archive_sets.c \
	string_hash.c \
	arch_state.c \
	arch_ctrl.c \
	log.c \
	util.c \
	key.c \
	list.c \
	init.c \
	types.c \
	au.c \
	fscmd.c \
	license.c \
	memory_free.c \
	master_config.c \
	parse_mcf.c \
	monitor.c \
	sam_wrap.c \
	stkapi.c \
	media_util.c \
	media.c \
	cfg_media.c \
	report.c \
	recycler.c \
	releaser.c \
	stager.c \
	parse_archiver.c \
	cfg_archiver.c \
	vsns.c \
	live_mount_options.c \
	vfstab.c \
	parse_samfs_cmd.c \
	mount_cfg.c \
	parser_utils.c \
	parse_stager.c \
	cfg_stager.c \
	parse_releaser.c \
	cfg_releaser.c \
	parse_diskvols.c \
	cfg_diskvols.c \
	diskvols.c \
	parse_recycler.c \
	cfg_recycler.c \
	archiver.c \
	notify_summary.c \
	filesystem.c \
	fs.c \
	load.c \
	stage.c \
	faults.c \
	recyc_sh_wrap.c \
	efilabel.c\
	readhost.c \
	sblk_mgmt.c \
	samrcsd.c \
	samrjobs.c \
	samrutil.c \
	samrdumps.c \
	cmn_csd_funcs.c \
	cmn_csd_restore.c \
	samrfunc.c \
	file_util.c \
	bswap.c \
	task_schedule.c \
	file_metrics_report.c \
	shrink_opts.c

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
	hosts_xdr.c \
	sammgmt_xdr.c \
	arch_set_xdr.c \
	restore_xdr.c \
	report_xdr.c \
	cmd_dispatch_xdr.c

ifndef NO_BUILD_CNS
LIB_SRC += csn.c
endif

# note that the -I for OBJ_DIR is required for version.h.
ARCHIVER_INCLUDES =	-I$(DEPTH)/src/archiver/include \
			-I$(INCLUDE)/aml/$(OBJ_DIR)

CATALOG_INCLUDES =     -I$(DEPTH)/src/catalog/include
# GCC make trouble for using this include, because there is a dirent.h file also
BSWAP_INCLUDES = -I$(DEPTH)/src/fs/include

STK_INCLUDES =	-I$(DEPTH)/src/robots/stk/stk_includes

# hosts_CPPFLAGS = $(BSWAP_INCLUDES)
# fs_CPPFLAGS = $(BSWAP_INCLUDES)
bswap_CPPFLAGS = $(BSWAP_INCLUDES)

DEPCFLAGS = -DAPI_DEBUG=syslog -DDEBUG -DSAM_LIB_GNU $(METADATA_SERVER) $(OSDEPCFLAGS) \
		-I. -I$(INCLUDE) -I$(INCLUDE)/pub/$(OBJ_DIR) -I$(DEPTH) \
		$(ARCHIVER_INCLUDES) \
		$(CATALOG_INCLUDES) \
		$(STK_INCLUDES) $(THRCOMP)

LIB_CSN_LIBS += -lpthread -lsocket -lnsl -ldl -lcsn

LIB_LIBS += 	-L$(DEPTH)/lib/$(OBJ_DIR) \
		-lsam -lsamut -lsamcat -lsamfs -lsamapi \
		-ladm -lgen -lelf -lz -lfsmdb \
		$(if $(filter $(NO_BUILD_CNS),),$(LIB_SCN_LIBS)) \
		$(DYNAMIC_OPT) $(LIBSO) 


DIRS = rpc 

ALWAYS_LINT_DEFS =      -errtags=yes -s
ALWAYS_LINT_DEFS +=     -erroff=E_PTRDIFF_OVERFLOW
ALWAYS_LINT_DEFS +=     -erroff=E_ASSIGN_NARROW_CONV
ALWAYS_LINT_DEFS +=     -U__PRAGMA_REDEFINE_EXTNAME
ALWAYS_LINT_DEFS +=     -Xc99=%all
ALWAYS_LINT_DEFS +=     -errsecurity=$(SECLEVEL)
ALWAYS_LINT_DEFS +=     -erroff=E_SEC_CREAT_WITHOUT_EXCL
ALWAYS_LINT_DEFS +=     -erroff=E_SEC_FORBIDDEN_WARN_CREAT
ALWAYS_LINT_DEFS +=     -erroff=E_ASSIGN_INT_TO_SMALL_INT
ALWAYS_LINT_DEFS +=     -erroff=E_CAST_INT_CONST_TO_SMALL_INT
ALWAYS_LINT_DEFS +=     -erroff=E_CAST_INT_TO_SMALL_INT
ALWAYS_LINT_DEFS +=     -erroff=E_CAST_TO_PTR_FROM_INT
ALWAYS_LINT_DEFS +=     -erroff=E_COMP_INT_WITH_LARGE_INT
ALWAYS_LINT_DEFS +=     -erroff=E_INTEGRAL_CONST_EXP_EXPECTED
ALWAYS_LINT_DEFS +=     -erroff=E_PASS_INT_TO_SMALL_INT
ALWAYS_LINT_DEFS +=     -erroff=E_PTR_CONV_LOSES_BITS

LNOPTS = -a -u -m -x
LNLIBS = -L$(DEPTH)/lib/$(OBJ_DIR)

# use the following for LNOPTS to get more detail from lint
#LNOPTS =  $(DEPCFLAGS) -m -x -errchk=%all -errfmt=macro -errsecurity=extended -fd -errtags=yes -erroff=E_SEC_PRINTF_VAR_FMT,E_OLD_STYLE_FUNC_DECL,E_FUNC_RET_ALWAYS_IGNOR,E_H_C_CHECK0,E_H_C_CHECK1,E_H_C_CHECK2,E_SEC_CREAT_WITHOUT_EXCL -Nlevel=4 -Ncheck=%all


include $(DEPTH)/mk/targets.mk

.NOTPARALLEL:

#
# $(LIB) needs to be built prior to building rcp/client and rpc/server
#
ifeq ($(MAKECMDGOALS), )
$(STRIP_DIRS):	$(LIB)
endif
ifeq ($(filter all,$(MAKECMDGOALS)), all)
$(STRIP_DIRS):	$(LIB)
endif

sammgmt:     robots

include $(DEPTH)/mk/depend.mk
