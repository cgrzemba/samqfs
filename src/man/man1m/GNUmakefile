# $Revision: 1.9 $

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

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

#  Keep the following list in alphabetical order, please.
MAN_SRCS =  archive_audit.1m archive_mark.1m archiver.1m archiver.sh.1m \
	arcopy.1m arfind.1m auditslot.1m build_cat.1m chmed.1m \
	cleandrive.1m clri.1m damage.1m dev_down.sh.1m   \
	dmpshm.1m dump_cat.1m dump_log.1m exarchive.1m export.1m    \
	generic.1m gnutar.1m import.1m samexplorer.1m   \
	itemize.1m load.1m load_notify.sh.1m log_rotate.sh.1m   \
	mccfg.1m mount_samfs.1m move.1m odlabel.1m  \
	qfsdump.1m qfsrestore.1m                                    \
	rearch.1m recover.sh.1m recycler.1m recycler.sh.1m          \
	releaser.1m reserve.1m restore.sh.1m robots.1m rpc.sam.1m   \
	sam-amld.1m sam-archiverd.1m sam-arcopy.1m sam-arfind.1m \
	sam-cld.1m sam-ftpd.1m sam-fsd.1m \
	sam-genericd.1m sam-ibm3494d.1m \
	sam-logd.1m sam-recycler.1m sam-releaser.1m \
	sam-rftd.1m sam-robotsd.1m sam-rpcd.1m sam-sonyd.1m       \
	sam-scannerd.1m sam-shrink.1m sam-stagealld.1m sam-stkd.1m \
	sam-stagerd.1m sam-stagerd_copy.1m \
	samtrace.1m samchaid.1m samcmd.1m samd.1m samdev.1m     \
	samgetvol.1m samexport.1m samfsck.1m samfsconfig.1m         \
	samfsdump.1m samfsinfo.1m samfsrestore.1m samfstyp.1m samgetmap.1m \
	samgetvol.1m samgrowfs.1m samload.1m sammkfs.1m             \
	samncheck.1m samquota.1m samquotastat.1m                    \
	samset.1m samsharefs.1m                                     \
	samu.1m samunhold.1m save_core.sh.1m scanner.1m \
	scsi_trace_decode.1m sefreport.1m                           \
	set_admin.1m set_state.1m                                   \
	showqueue.1m stageall.1m stageback.sh.1m star.1m            \
	tarback.sh.1m tplabel.1m                                    \
	umount_samfs.1m unarchive.1m undamage.1m                    \
	unload.1m unrearch.1m unreserve.1m

MAN_RELEASED_SRCS = $( shell echo archive_audit.1m \
	archiver.sh.1m auditslot.1m build_cat.1m chmed.1m cleandrive.1m \
	dev_down.sh.1m dmpshm.1m         \
	dump_cat.1m dump_log.1m exarchive.1m export.1m          \
	gnutar.1m import.1m samexplorer.1m itemize.1m                   \
	load.1m load_notify.sh.1m log_rotate.sh.1m \
	mccfg.1m mount_samfs.1m move.1m \
	odlabel.1m qfsdump.1m rearch.1m             \
	recover.sh.1m recycler.sh.1m reserve.1m restore.sh.1m   \
	sam-amld.1m sam-archiverd.1m sam-arcopy.1m sam-arfind.1m sam-ftpd.1m \
	sam-fsd.1m sam-logd.1m sam-recycler.1m sam-releaser.1m \
	sam-rftd.1m sam-robotsd.1m sam-rpcd.1m sam-scannerd.1m  \
	sam-shrink.1m sam-stagerd.1m sam-stagerd_copy.1m \
	sam-stagealld.1m samtrace.1m samunhold.1m sam-sonyd.1m      \
	samchaid.1m samcmd.1m samd.1m samdev.1m samfsck.1m          \
	samfsconfig.1m samfsdump.1m samfstyp.1m             \
	samgrowfs.1m sammkfs.1m samncheck.1m                        \
	samquota.1m samquotastat.1m                                 \
	samset.1m samsharefs.1m                                     \
	samu.1m save_core.sh.1m sefreport.1m            \
	set_admin.1m set_state.1m showqueue.1m                      \
	stageback.sh.1m star.1m tarback.sh.1m tplabel.1m            \
	umount_samfs.1m unarchive.1m                                \
	undamage.1m unload.1m unrearch.1m unreserve.1m              \
	| tr " " "\012" | sort )

QFS_RELEASED_SRCS = $( shell echo mount_samfs.1m	                 \
	qfsdump.1m                                                  \
	samd.1m sam-fsd.1m                                          \
	samtrace.1m samchaid.1m samcmd.1m samfsck.1m samfsconfig.1m \
	samfstyp.1m samgrowfs.1m sammkfs.1m samncheck.1m                        \
	samquota.1m samquotastat.1m samsharefs.1m                   \
	samu.1m                                                     \
	samunhold.1m                                                \
	umount_samfs.1m                                             \
	| tr " " "\012" | sort )

REM_RELEASED_SRCS = sam-robotsd.1m

include $(DEPTH)/mk/targets_man.mk

include $(DEPTH)/mk/depend.mk
