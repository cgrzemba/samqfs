/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: SelectableGroupHelper.java,v 1.28 2008/03/17 14:40:44 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.util.Constants;

public interface SelectableGroupHelper {
    public static final String NOVAL_LABEL = "--";
    public static final String NOVAL = new Integer(-999).toString();

    public interface ScanMethod {
        public static final String [] labels = {
            NOVAL_LABEL,
            "ArchiveSetup.noscan",
            "ArchiveSetup.contscan",
            "ArchiveSetup.scandirs",
            "ArchiveSetup.scaninodes"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(GlobalArchiveDirective.NO_SCAN).toString(),
            new Integer(GlobalArchiveDirective.CONTINUOUS_SCAN).toString(),
            new Integer(GlobalArchiveDirective.SCAN_DIRS).toString(),
            new Integer(GlobalArchiveDirective.SCAN_INODES).toString()
        };
    }

    public interface Time {
        public static final String [] labels = {
            "common.unit.time.weeks",
            "common.unit.time.days",
            "common.unit.time.hours",
            "common.unit.time.minutes",
            "common.unit.time.seconds"
        };
        public static final String [] values = {
            new Integer(SamQFSSystemModel.TIME_WEEK).toString(),
            new Integer(SamQFSSystemModel.TIME_DAY).toString(),
            new Integer(SamQFSSystemModel.TIME_HOUR).toString(),
            new Integer(SamQFSSystemModel.TIME_MINUTE).toString(),
            new Integer(SamQFSSystemModel.TIME_SECOND).toString()
        };
    }

    public interface Size {
        public static final String [] labels = {
            NOVAL_LABEL,
            "common.unit.size.b",
            "common.unit.size.kb",
            "common.unit.size.mb",
            "common.unit.size.gb",
            "common.unit.size.tb",
            "common.unit.size.pb"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(SamQFSSystemModel.SIZE_B).toString(),
            new Integer(SamQFSSystemModel.SIZE_KB).toString(),
            new Integer(SamQFSSystemModel.SIZE_MB).toString(),
            new Integer(SamQFSSystemModel.SIZE_GB).toString(),
            new Integer(SamQFSSystemModel.SIZE_TB).toString(),
            Integer.toString(SamQFSSystemModel.SIZE_PB)
        };
    }

    public interface Times {
        public static final String [] labels = {
            "common.unit.time.weeks",
            "common.unit.time.days",
            "common.unit.time.hours",
            "common.unit.time.minutes",
            "common.unit.time.seconds"
        };
        public static final String [] values = {
            new Integer(SamQFSSystemModel.TIME_WEEK).toString(),
            new Integer(SamQFSSystemModel.TIME_DAY).toString(),
            new Integer(SamQFSSystemModel.TIME_HOUR).toString(),
            new Integer(SamQFSSystemModel.TIME_MINUTE).toString(),
            new Integer(SamQFSSystemModel.TIME_SECOND).toString()
        };
    }

    public interface ExpirationTime {
        public static final String [] labels = {
            "common.unit.time.days",
            "common.unit.time.weeks",
            "common.unit.time.years"
        };
        public static final String [] values = {
            new Integer(SamQFSSystemModel.TIME_DAY).toString(),
            new Integer(SamQFSSystemModel.TIME_WEEK).toString(),
            new Integer(SamQFSSystemModel.TIME_YEAR).toString()
        };
    }

    public interface Sizes {
        public static final String [] labels = {
            "common.unit.size.b",
            "common.unit.size.kb",
            "common.unit.size.mb",
            "common.unit.size.gb",
            "common.unit.size.tb",
            "common.unit.size.pb"
        };

        public static final String [] values = {
            new Integer(SamQFSSystemModel.SIZE_B).toString(),
            new Integer(SamQFSSystemModel.SIZE_KB).toString(),
            new Integer(SamQFSSystemModel.SIZE_MB).toString(),
            new Integer(SamQFSSystemModel.SIZE_GB).toString(),
            new Integer(SamQFSSystemModel.SIZE_TB).toString(),
            Integer.toString(SamQFSSystemModel.SIZE_PB)
        };
    }

    public interface Staging {
        public static final String [] labels = {
            NOVAL_LABEL,
            "archiving.criteria.staging.associative",
            "archiving.criteria.staging.never",
            "archiving.criteria.staging.defaults"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(ArchivePolicy.STAGE_ASSOCIATIVE).toString(),
            new Integer(ArchivePolicy.STAGE_NEVER).toString(),
            new Integer(ArchivePolicy.STAGE_DEFAULTS).toString()
        };
    }

    public interface StagingForWizard {
        public static final String [] labels = {
            "NewCriteriaWizard.criteria.staging.auto",
            "NewCriteriaWizard.criteria.staging.associative",
            "NewCriteriaWizard.criteria.staging.never",
            "NewCriteriaWizard.criteria.staging.defaults"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(ArchivePolicy.STAGE_ASSOCIATIVE).toString(),
            new Integer(ArchivePolicy.STAGE_NEVER).toString(),
            new Integer(ArchivePolicy.STAGE_DEFAULTS).toString()
        };
    }

    public interface Releasing {
        public static final String [] labels = {
            NOVAL_LABEL,
            "archiving.criteria.releasing.never",
            "archiving.criteria.releasing.partial",
            "archiving.criteria.releasing.onecopy",
            "archiving.criteria.releasing.defaults"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(ArchivePolicy.RELEASE_NEVER).toString(),
            new Integer(ArchivePolicy.RELEASE_PARTIAL).toString(),
            new Integer(ArchivePolicy.RELEASE_AFTER_ONE).toString(),
            new Integer(ArchivePolicy.RELEASE_DEFAULTS).toString()
        };
    }

    public interface ReleasingForWizard {
        public static final String [] labels = {
            "NewCriteriaWizard.criteria.releasing.reached",
            "NewCriteriaWizard.criteria.releasing.never",
            "NewCriteriaWizard.criteria.releasing.partial",
            "NewCriteriaWizard.criteria.releasing.onecopy",
            "NewCriteriaWizard.criteria.releasing.defaults"
        };

        public static final String [] values = {
            NOVAL,
            new Integer(ArchivePolicy.RELEASE_NEVER).toString(),
            new Integer(ArchivePolicy.RELEASE_PARTIAL).toString(),
            new Integer(ArchivePolicy.RELEASE_AFTER_ONE).toString(),
            new Integer(ArchivePolicy.RELEASE_DEFAULTS).toString()
        };
    }

    public interface MigrateFromPool {
        public static final String [] labels = {
            "archiving.dataclass.migratefrom.default",
            "archiving.dataclass.migratefrom.afterone",
            "archiving.dataclass.migratefrom.partial",
            "archiving.dataclass.migratefrom.never",
        };

        public static final String [] values = {
            Integer.toString(ArchivePolicy.RELEASE_NO_OPTION_SET),
            new Integer(ArchivePolicy.RELEASE_AFTER_ONE).toString(),
            new Integer(ArchivePolicy.RELEASE_PARTIAL).toString(),
            new Integer(ArchivePolicy.RELEASE_NEVER).toString()
        };
    }

    public interface MigrateToPool {
        public static final String [] labels = {
            "archiving.dataclass.migrateto.default",
            "archiving.dataclass.migrateto.associative"
        };

        public static final String [] values = {
            Integer.toString(ArchivePolicy.STAGE_NO_OPTION_SET),
            new Integer(ArchivePolicy.STAGE_ASSOCIATIVE).toString(),
        };
    }



    public interface ReleaseOptions {
        public static final String [] labels = {
            "ArchivePolCopy.general.spaceRequired",
            "ArchivePolCopy.general.release",
            "ArchivePolCopy.general.noRelease"
        };

        public static final String [] values = {
            NOVAL,
            "true",
            "false"
        };
    }


    public interface ReservationMethod {
        public static final String [] labels = {
            NOVAL_LABEL,
            "archiving.reservation.method.dir",
            "archiving.reservation.method.user",
            "archiving.reservation.method.group"
        };

        public static final String [] values = {
            NOVAL,
            Integer.toString(ReservationMethodHelper.RM_DIR),
            Integer.toString(ReservationMethodHelper.RM_USER),
            Integer.toString(ReservationMethodHelper.RM_GROUP)
        };
    };

    public interface UATimeReference {
        public static final String [] labels = {
            "ArchivePolCopy.general.access",
            "ArchivePolCopy.general.modification"
        };

        public static final String [] values = {
            Integer.toString(ArchivePolicy.UNARCH_TIME_REF_ACCESS),
            Integer.toString(ArchivePolicy.UNARCH_TIME_REF_MODIFICATION)
        };
    };

    public interface SortMethod {
        public static final String [] labels = {
            "ArchivePolCopy.basic.none",
            "ArchivePolCopy.basic.age",
            "ArchivePolCopy.basic.path",
            "ArchivePolCopy.basic.priority",
            "ArchivePolCopy.basic.size"
        };

        public static final String [] values = {
            Integer.toString(ArchivePolicy.SM_NONE),
            Integer.toString(ArchivePolicy.SM_AGE),
            Integer.toString(ArchivePolicy.SM_PATH),
            Integer.toString(ArchivePolicy.SM_PRIORITY),
            Integer.toString(ArchivePolicy.SM_SIZE)
        };
    }

    public interface OfflineCopyMethod {
        public static final String [] labels = {
            "NewPolicyWizard.tapecopyoption.offlineCopy.default",
            "NewPolicyWizard.tapecopyoption.offlineCopy.direct",
            "NewPolicyWizard.tapecopyoption.offlineCopy.stageAhead",
            "NewPolicyWizard.tapecopyoption.offlineCopy.stageAll"
        };

        public static final String [] values = {
            NOVAL,
            Integer.toString(ArchivePolicy.OC_DIRECT),
            Integer.toString(ArchivePolicy.OC_STAGEAHEAD),
            Integer.toString(ArchivePolicy.OC_STAGEALL)
        };
    };


    public interface RecyclingOptions {
        public static final String [] labels = {
            "NewArchivePolWizard.page5.ignoreRecyclingOption1",
            "NewArchivePolWizard.page5.ignoreRecyclingOption2"
        };

        public static final String [] values = {
            Constants.Archive.RECYCLING_OPTION_YES,
            Constants.Archive.RECYCLING_OPTION_NO
        };
    }

    public interface JoinMethod {
        public static final String [] labels = {
            "ArchivePolCopy.general.noJoin",
            "ArchivePolCopy.general.joinPath"
        };

        public static final String [] values = {
            Integer.toString(ArchivePolicy.NO_JOIN),
            Integer.toString(ArchivePolicy.JOIN_PATH)
        };
    }

    public interface saveOptions {
        public static final String [] labels = {
            "NewArchivePolWizard.page7.commit",
            "NewArchivePolWizard.page7.save"
        };

        public static final String [] values = {
            "NewArchivePolWizard.page7.commit",
            "NewArchivePolWizard.page7.save"
        };
    }

    public interface copyNumber {
        public static final String [] labels = {
            "1",
            "2",
            "3",
            "4"
        };

        public static final String [] values = {
            new Integer(1).toString(),
            new Integer(2).toString(),
            new Integer(3).toString(),
            new Integer(4).toString()
        };
    }

    public interface archiveActions {
        public static final String [] labels = {
            "ArchiveActivity.restart",
            "ArchiveActivity.idle",
            "ArchiveActivity.runnow",
            "ArchiveActivity.rerun",
            "ArchiveActivity.stop"
        };

        public static final String [] values = {
            String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_RESTART),
            String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_IDLE),
            String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_RUN),
            String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_RERUN),
            String.valueOf(SamQFSSystemModel.ACTIVITY_ARCHIVE_STOP)
        };
    }

    public interface stageActions {
        public static final String [] labels = {
            "ArchiveActivity.idle",
            "ArchiveActivity.runnow"
        };

        public static final String [] values = {
            String.valueOf(SamQFSSystemModel.ACTIVITY_STAGE_IDLE),
            String.valueOf(SamQFSSystemModel.ACTIVITY_STAGE_RUN)
        };
    }

    public interface namePattern {
        public static final String [] labels = {
            "archiving.dataclass.namepattern.regex",
            "archiving.dataclass.namepattern.filecontains",
            "archiving.dataclass.namepattern.pathcontains",
            "archiving.dataclass.namepattern.endswith"
        };

        public static final String [] values = {
            String.valueOf(Criteria.REGEXP),
            String.valueOf(Criteria.FILE_NAME_CONTAINS),
            String.valueOf(Criteria.PATH_CONTAINS),
            String.valueOf(Criteria.ENDS_WITH),
        };
    }

    public interface refreshInterval {
        public static final String [] labels = {
            "Monitor.timeinterval.off",
            "Monitor.timeinterval.seconds.ten",
            "Monitor.timeinterval.seconds.thirty",
            "Monitor.timeinterval.minutes.one",
            "Monitor.timeinterval.minutes.two",
            "Monitor.timeinterval.minutes.five",
            "Monitor.timeinterval.minutes.ten",
            "Monitor.timeinterval.minutes.thirty"
        };

        public static final String [] values = {
            Integer.toString(-1),
            Integer.toString(10),
            Integer.toString(30),
            Integer.toString(60),
            Integer.toString(120),
            Integer.toString(300),
            Integer.toString(600),
            Integer.toString(1800)
        };
    }

    public interface periodicAuditSetting {
        public static final String [] labels = {
            "archiving.dataclass.copyaudit.none",
            "archiving.dataclass.copyaudit.diskonly",
            "archiving.dataclass.copyaudit.diskandtape"
        };

        public static final String [] values = {
            Integer.toString(0),
            Integer.toString(1),
            Integer.toString(2)
        };
    }
}
