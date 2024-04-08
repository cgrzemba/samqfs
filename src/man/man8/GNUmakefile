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

#  Keep the following list in alphabetical order, please.
MAN_SRCS =  archive_audit.8 archive_mark.8 archiver.8 archiver.sh.8 \
	arcopy.8 arfind.8 auditslot.8 build_cat.8 chmed.8 \
	cleandrive.8 clri.8 damage.8 dev_down.sh.8   \
	dmpshm.8 dump_cat.8 dump_log.8 exarchive.8 export.8    \
	generic.8 gnutar.8 samimport.8 import.8 samexplorer.8   \
	itemize.8 load.8 load_notify.sh.8 log_rotate.sh.8   \
	mccfg.8 mount_samfs.8 move.8 odlabel.8  \
	qfsdump.8 qfsrestore.8                                    \
	rearch.8 recover.sh.8 recycler.8 recycler.sh.8          \
	releaser.8 reserve.8 restore.sh.8 robots.8 rpc.sam.8   \
	sam-amld.8 sam-archiverd.8 sam-arcopy.8 sam-arfind.8 \
	sam-cld.8 sam-ftpd.8 sam-fsd.8 \
	sam-genericd.8 sam-ibm3494d.8 \
	sam-logd.8 sam-recycler.8 sam-releaser.8 \
	sam-rftd.8 sam-robotsd.8 sam-rpcd.8 sam-sonyd.8       \
	sam-scannerd.8 sam-shrink.8 sam-stagealld.8 sam-stkd.8 \
	sam-stagerd.8 sam-stagerd_copy.8 \
	samtrace.8 samchaid.8 samcmd.8 samd.8 samdb.8 samdev.8     \
	samgetvol.8 samexport.8 samfsck.8 samfsconfig.8         \
	samfsdump.8 samfsinfo.8 samfsrestore.8 samfstyp.8 samgetmap.8 \
	samgetvol.8 samgrowfs.8 samload.8 sammkfs.8             \
	samncheck.8 samquota.8 samquotastat.8                    \
	samset.8 samsharefs.8                                     \
	samu.8 samunhold.8 save_core.sh.8 scanner.8 \
	scsi_trace_decode.8 sefreport.8                           \
	set_admin.8 set_state.8                                   \
	showqueue.8 stageall.8 stageback.sh.8 star.8            \
	tarback.sh.8 tplabel.8                                    \
	umount_samfs.8 unarchive.8 undamage.8                    \
	unload.8 unrearch.8 unreserve.8

MAN_RELEASED_SRCS = $( shell echo archive_audit.8 \
	archiver.sh.8 auditslot.8 build_cat.8 chmed.8 cleandrive.8 \
	dev_down.sh.8 dmpshm.8         \
	dump_cat.8 dump_log.8 exarchive.8 export.8          \
	gnutar.8 import.8 samexplorer.8 itemize.8                   \
	load.8 load_notify.sh.8 log_rotate.sh.8 \
	mccfg.8 mount_samfs.8 move.8 \
	odlabel.8 qfsdump.8 rearch.8             \
	recover.sh.8 recycler.sh.8 reserve.8 restore.sh.8   \
	sam-amld.8 sam-archiverd.8 sam-arcopy.8 sam-arfind.8 sam-ftpd.8 \
	sam-fsd.8 sam-logd.8 sam-recycler.8 sam-releaser.8 \
	sam-rftd.8 sam-robotsd.8 sam-rpcd.8 sam-scannerd.8  \
	sam-shrink.8 sam-stagerd.8 sam-stagerd_copy.8 \
	sam-stagealld.8 samtrace.8 samunhold.8 sam-sonyd.8      \
	samchaid.8 samcmd.8 samd.8 samdb.8 samdev.8 samfsck.8          \
	samfsconfig.8 samfsdump.8 samfstyp.8             \
	samgrowfs.8 sammkfs.8 samncheck.8                        \
	samquota.8 samquotastat.8                                 \
	samset.8 samsharefs.8                                     \
	samu.8 save_core.sh.8 sefreport.8            \
	set_admin.8 set_state.8 showqueue.8                      \
	stageback.sh.8 star.8 tarback.sh.8 tplabel.8            \
	umount_samfs.8 unarchive.8                                \
	undamage.8 unload.8 unrearch.8 unreserve.8              \
	| tr " " "\012" | sort )

QFS_RELEASED_SRCS = $( shell echo mount_samfs.8	                 \
	qfsdump.8                                                  \
	samd.8 sam-fsd.8                                          \
	samtrace.8 samchaid.8 samcmd.8 samfsck.8 samfsconfig.8 \
	samfstyp.8 samgrowfs.8 sammkfs.8 samncheck.8                        \
	samquota.8 samquotastat.8 samsharefs.8                   \
	samu.8                                                     \
	samunhold.8                                                \
	umount_samfs.8                                             \
	| tr " " "\012" | sort )

REM_RELEASED_SRCS = sam-robotsd.8

include $(DEPTH)/mk/targets_man.mk

include $(DEPTH)/mk/depend.mk
