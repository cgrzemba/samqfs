# $Revision: 1.10 $

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

MAN_SRCS =  intro_libsam.3 qfs_listio.3 \
	sam_advise.3 sam_archive.3 sam_audit.3 sam_cancelstage.3        \
	sam_chmed.3 sam_clear_request.3 sam_closecat.3 sam_devstat.3        \
	sam_devstr.3 sam_errno.3 sam_export.3 sam_getcatalog.3              \
	sam_getfsdata.3 sam_getfsdisks.3 sam_import.3 sam_load.3            \
	sam_lstat.3 sam_mig_create_file.3 sam_mig_mount_media.3             \
	sam_mig_rearchive.3 sam_mig_release_device.3 sam_mig_stage_end.3    \
	sam_mig_stage_error.3 sam_mig_stage_file.3 sam_mig_stage_write.3    \
	sam_move.3 sam_odlabel.3 sam_opencat.3 sam_readrminfo.3             \
	sam_release.3 sam_request.3 sam_restore_file.3 sam_restore_copy.3   \
	sam_set_fs_contig.3 sam_set_fs_thresh.3 sam_set_state.3             \
	sam_setfa.3 sam_settings.3 sam_ssum.3 sam_stage.3 sam_stat.3        \
	sam_segment.3 sam_segment_stat.3 sam_tplabel.3 sam_unload.3     \
	sam_vsn_stat.3  sam_segment_vsn_stat.3 usam_mig_cancel_stage_req.3  \
	usam_mig_initialize.3 usam_mig_stage_file_req.3

MAN_RELEASED_SRCS = intro_libsam.3 qfs_listio.3 \
	sam_advise.3 sam_archive.3 sam_cancelstage.3            \
	sam_closecat.3 sam_devstat.3 sam_devstr.3 sam_getcatalog.3          \
	sam_opencat.3 sam_readrminfo.3 sam_release.3 sam_request.3          \
	sam_restore_file.3 sam_restore_copy.3                               \
	sam_segment.3 sam_setfa.3 sam_ssum.3 sam_stage.3 sam_stat.3         \
	sam_segment_stat.3 sam_vsn_stat.3 sam_segment_vsn_stat.3

MIG_RELEASED_SRCS = intro_libsam.3 \
	sam_mig_create_file.3 sam_mig_mount_media.3         \
	sam_mig_rearchive.3 sam_mig_release_device.3 sam_mig_stage_end.3    \
	sam_mig_stage_error.3 sam_mig_stage_file.3 sam_mig_stage_write.3    \
	usam_mig_cancel_stage_req.3 usam_mig_initialize.3                   \
	usam_mig_stage_file_req.3

QFS_RELEASED_SRCS = intro_libsam.3 qfs_listio.3 sam_advise.3 sam_setfa.3

include $(DEPTH)/mk/targets_man.mk

include $(DEPTH)/mk/depend.mk
