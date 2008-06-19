# $Revision: 1.39 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..
LIB_HOME = $(DEPTH)/src/lib/sammgmtjni
JAR_HOME = ../..

include $(DEPTH)/mk/common.mk

NATIVEHOME = .
JAVA_HOME = $(JBASE)
JNIH = $(OBJ_DIR)/jnih

LIB = fsmgmtjni
LIB_SRC = \
	$(NATIVEHOME)/jni_util.c \
	$(NATIVEHOME)/conn.c \
	$(NATIVEHOME)/jni_dev.c \
	$(NATIVEHOME)/jni_fs.c \
	$(NATIVEHOME)/jni_dvol.c \
	$(NATIVEHOME)/jni_media.c \
	$(NATIVEHOME)/jni_vsn.c \
	$(NATIVEHOME)/jni_arc.c \
	$(NATIVEHOME)/jni_releaser.c \
	$(NATIVEHOME)/jni_stager.c \
	$(NATIVEHOME)/jni_recycler.c \
	$(NATIVEHOME)/jni_faults.c \
	$(NATIVEHOME)/jni_notif.c \
	$(NATIVEHOME)/jni_hosts.c \
	$(NATIVEHOME)/jni_misc.c \
	${NATIVEHOME}/jni_restore.c \
	${NATIVEHOME}/jni_file_util.c \
	${NATIVEHOME}/jni_reg.c \
	$(NATIVEHOME)/jni_task_sched.c

DEPCFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/solaris -I$(JNIH) \
	-I$(DEPTH)/include -I$(DEPTH)/include/pub -DAPI_DEBUG=syslog $(THRCOMP)
LIB_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -mt -lfsmgmtrpc

BASEPKG = com/sun/netstorage/samqfs/mgmt

JFLAGS =
JC = $(JAVA_HOME)/bin/javac -deprecation -classpath . -d $(OBJ_DIR)
JH = $(JAVA_HOME)/bin/javah -classpath $(OBJ_DIR) -d $(JNIH)
JSRCS = \
	$(BASEPKG)/SamFSException.java \
	$(BASEPKG)/SamFSCommException.java \
	$(BASEPKG)/SamFSMultiMsgException.java \
	$(BASEPKG)/SamFSMultiHostException.java \
	$(BASEPKG)/SamFSIncompatVerException.java \
	$(BASEPKG)/SamFSMultiStepOpException.java \
	$(BASEPKG)/SamFSTimeoutException.java \
	$(BASEPKG)/SamFSAccessDeniedException.java \
	$(BASEPKG)/SamFSWarnings.java \
	$(BASEPKG)/Auditable.java \
	$(BASEPKG)/Ctx.java \
	$(BASEPKG)/BaseDev.java \
        $(BASEPKG)/reg/Register.java \
        $(BASEPKG)/reg/SimpleSignature.java \
        $(BASEPKG)/reg/DH.java \
	$(BASEPKG)/SamFSConnection.java \
	$(BASEPKG)/arc/Copy.java \
	$(BASEPKG)/arc/Criteria.java \
	$(BASEPKG)/arc/BufDirective.java \
	$(BASEPKG)/arc/DrvDirective.java \
	$(BASEPKG)/arc/ArGlobalDirective.java \
	$(BASEPKG)/arc/ArFSDirective.java \
	$(BASEPKG)/arc/ArSet.java \
	$(BASEPKG)/arc/ArPriority.java \
	$(BASEPKG)/arc/CopyParams.java \
	$(BASEPKG)/arc/VSNPool.java \
	$(BASEPKG)/arc/VSNMap.java \
	$(BASEPKG)/arc/VSNPoolProps.java \
	$(BASEPKG)/arc/BaseVSNPoolProps.java \
	$(BASEPKG)/arc/CatVSNPoolProps.java \
	$(BASEPKG)/arc/DiskVSNPoolProps.java \
	$(BASEPKG)/arc/job/ArCopyProc.java \
	$(BASEPKG)/arc/job/ArCopyJob.java \
	$(BASEPKG)/arc/job/Stats.java \
	$(BASEPKG)/arc/job/ArFindFsStats.java \
	$(BASEPKG)/fs/FSInfo.java \
	$(BASEPKG)/fs/SCSIDevInfo.java \
	$(BASEPKG)/fs/DiskDev.java \
	$(BASEPKG)/fs/StripedGrp.java \
	$(BASEPKG)/fs/MountOptions.java \
	$(BASEPKG)/fs/EQ.java \
	$(BASEPKG)/fs/FSArchCfg.java \
	$(BASEPKG)/media/ImportOpts.java \
	$(BASEPKG)/media/DriveDev.java \
	$(BASEPKG)/media/MdLicense.java \
	$(BASEPKG)/media/LibDev.java \
	$(BASEPKG)/media/Discovered.java \
	$(BASEPKG)/media/ReservInfo.java \
	$(BASEPKG)/media/CatEntry.java \
	$(BASEPKG)/media/NetAttachLibInfo.java \
	$(BASEPKG)/media/NetLibParam.java \
	$(BASEPKG)/media/StkNetLibParam.java \
	$(BASEPKG)/media/StkCell.java \
	$(BASEPKG)/media/StkClntConn.java \
	$(BASEPKG)/media/StkPool.java \
	$(BASEPKG)/media/StkVSN.java \
	$(BASEPKG)/media/StkCap.java \
	$(BASEPKG)/media/StkCapacity.java \
	$(BASEPKG)/media/StkDevice.java \
	$(BASEPKG)/media/StkPhyConf.java \
	$(BASEPKG)/rel/ReleaserDirective.java \
	$(BASEPKG)/rec/RecyclerParams.java \
	$(BASEPKG)/rec/LibRecParams.java \
	$(BASEPKG)/adm/FaultAttr.java \
	$(BASEPKG)/adm/FaultSummary.java \
	$(BASEPKG)/stg/StagerParams.java \
	$(BASEPKG)/stg/job/StgFileInfo.java

JNATIVESRCS = \
	$(BASEPKG)/SamFSConnection.java \
	$(BASEPKG)/Job.java \
	$(BASEPKG)/FileUtil.java \
	$(BASEPKG)/GetList.java \
	$(BASEPKG)/Util.java \
	$(BASEPKG)/fs/AU.java \
	$(BASEPKG)/fs/FS.java \
	$(BASEPKG)/fs/SamfsckJob.java \
	$(BASEPKG)/fs/Host.java \
	${BASEPKG}/fs/Restore.java \
	$(BASEPKG)/media/Media.java \
	$(BASEPKG)/media/MountJob.java \
	$(BASEPKG)/media/LabelJob.java \
	$(BASEPKG)/arc/VSNOp.java \
	$(BASEPKG)/arc/DiskVol.java \
	$(BASEPKG)/arc/Archiver.java \
	$(BASEPKG)/arc/job/ArchReq.java \
	$(BASEPKG)/arc/job/ArFindJob.java \
	$(BASEPKG)/rel/Releaser.java \
	$(BASEPKG)/rel/ReleaserJob.java \
	$(BASEPKG)/stg/Stager.java \
	$(BASEPKG)/stg/job/StagerStream.java \
	$(BASEPKG)/stg/job/StagerJob.java \
	$(BASEPKG)/rec/Recycler.java \
	$(BASEPKG)/reg/Register.java \
	$(BASEPKG)/adm/Fault.java \
	$(BASEPKG)/adm/NotifSummary.java \
	$(BASEPKG)/adm/LastJob.java \
	$(BASEPKG)/adm/License.java \
	$(BASEPKG)/adm/Report.java \
	$(BASEPKG)/adm/SysInfo.java \
	$(BASEPKG)/adm/TaskSchedule.java

JTESTSRCS = \
	${BASEPKG}/test/TUtil.java \
	${BASEPKG}/test/TestFSShrink.java \
	${BASEPKG}/test/TestVSNPoolProps.java \
	${BASEPKG}/test/TestDVolFlags.java \
	${BASEPKG}/test/TestFScreate.java \
	${BASEPKG}/test/TestTaskSchedule.java \
	${BASEPKG}/test/TestArSet.java \
	${BASEPKG}/test/TestReports.java \
	${BASEPKG}/test/TestStageFiles.java \
	${BASEPKG}/test/TestFileUtil.java \
	${BASEPKG}/test/TestStkMedia.java \
	${BASEPKG}/test/TestNFSgetOpt.java \
	${BASEPKG}/test/TestActivitygetall.java \
	${BASEPKG}/test/TestSysInfo.java \
	${BASEPKG}/test/TestFSEq.java \
	${BASEPKG}/test/TestFSget.java \
	${BASEPKG}/test/TestFSgetall.java \
	${BASEPKG}/test/TestFSmount.java \
	${BASEPKG}/test/TestFSumount.java \
	${BASEPKG}/test/TestFSgrow.java \
	${BASEPKG}/test/TestFSremove.java \
	${BASEPKG}/test/TestFSchk.java \
	${BASEPKG}/test/TestHost.java \
	${BASEPKG}/test/TestArcGetarfsd.java \
	${BASEPKG}/test/TestArcResetarfsd.java \
	${BASEPKG}/test/TestArcGlob.java \
	${BASEPKG}/test/TestArcJobs.java \
	${BASEPKG}/test/TestActArcCfg.java \
	${BASEPKG}/test/TestCrit.java \
	${BASEPKG}/test/TestCParams.java \
	${BASEPKG}/test/TestConn.java \
	${BASEPKG}/test/TestAUDisco.java \
	${BASEPKG}/test/TestMntOpts.java \
	${BASEPKG}/test/TestMedia.java \
	${BASEPKG}/test/TestMediaGet.java \
	${BASEPKG}/test/TestMediaOp.java \
	${BASEPKG}/test/TestMediaMntJobs.java \
	${BASEPKG}/test/TestVSNMap.java \
	${BASEPKG}/test/TestVSNPool.java \
	${BASEPKG}/test/TestRel.java \
	${BASEPKG}/test/TestRelJobs.java \
	${BASEPKG}/test/TestRec.java \
	${BASEPKG}/test/TestStager.java \
	${BASEPKG}/test/TestStgJobs.java \
	${BASEPKG}/test/TestLabelJobs.java \
	${BASEPKG}/test/TestFaultsGet.java \
	${BASEPKG}/test/TestCatalog.java \
	${BASEPKG}/test/TestNotif.java \
	${BASEPKG}/test/TestLicense.java \
	${BASEPKG}/test/TestJobCancel.java \
	${BASEPKG}/test/TestRestoreDumps.java


JAVASRCS = $(JSRCS) $(JNATIVESRCS)

DOT_NAMES = $(shell echo $(JNATIVESRCS:%.java=%.class) | tr '/' '.')
JAVAHDRS = $(DOT_NAMES:.class=)

JAVAOBJS = $(JNATIVESRCS:%.java=$(OBJ_DIR)/%.class)
JAVAOBJS += $(JAVASRCS:%.java=$(OBJ_DIR)/%.class)
JTSTOBJS = $(JTESTSRCS:%.java=$(OBJ_DIR)/%.class)

JAVA_TARGETS = java_lib

LNOPTS = -a -u -m -x
LNLIBS =

include $(DEPTH)/mk/targets.mk

.INIT:	$(JNIH) java_hdrs

$(JNIH):
	mkdir -p $(JNIH)

clean:	clean_java_local
#
# TO BE REMOVED: -rm -f $(DEPTH)/jar/$(LIB).jar
# Temporary for release 4.5.q. From 4.5.q, this jar gets
# created only in $(OBJ_DIR)/$(LIB).jar
#
clean_java_local:
	-rm -rf $(JNIH)
	-rm -f $(DEPTH)/jar/$(LIB).jar
	-rm -f $(OBJ_DIR)/$(LIB).jar

test:  ${JTSTOBJS}

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
