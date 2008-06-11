/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
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

// ident	$Id: Constants.java,v 1.104 2008/06/11 16:58:01 ronaldso Exp $

package com.sun.netstorage.samqfs.web.util;

/**
 *  Constants defined for SAM-QFS Manager Application.
 */

public interface Constants {

    public interface Alarm {
        public static final String ALARM_ALL  = "ALL";
        public static final String ALARM_DOWN = "DOWN";
        public static final String ALARM_CRITICAL = "CRITICAL";
        public static final String ALARM_MAJOR = "MAJOR";
        public static final String ALARM_MINOR = "MINOR";
    }

    public interface Config {
        public static final String
            ARCHIVE_CONFIG =  "Error in archiver configuration";
        public static final String
            ARCHIVE_CONFIG_WARNING = "Warning in archiver configuration";
    }

    public interface ProductNameDim {
        public static final String WIDTH  = "170";
        public static final String HEIGHT = "20";
    }

    public interface AlertKeys {
        public static final String OPERATION_RESULT  = "Finish_result";
        public static final String OPERATION_SUCCESS = "successful";
        public static final String OPERATION_FAILED  = "Failed";
        public static final String OPERATION_WARNING = "Warning";
        public static final String
            OPERATION_FAILED_MULTISTEP = "FAILED_MULTISTEP";

        public static final String SUMMARY_MESSAGE = "SUMMARY_MESSAGE";
        public static final String DETAIL_MESSAGE  = "DETAIL_MESSAGE";
        public static final String
            MULTISTEP_EXCEPTION_CODE = "MULTISTEP_EXCEPTION_CODE";
    }

    public interface ResourceProperty {
        public static final String
            BASE_NAME = "com.sun.netstorage.samqfs.web.resources.Resources";
    }

    public interface Masthead {
        public static final String PRODUCT_NAME_SRC =
            "masthead.logo";
        public static final String PRODUCT_NAME_ALT =
            "masthead.altText";
    }
    public interface SecondaryMasthead {
        public static final String PRODUCT_NAME_SRC =
            "secondaryMasthead.productNameSrc";
        public static final String PRODUCT_NAME_ALT =
            "secondaryMasthead.productNameAlt";
    }

    public interface BreadCrumb {
        public static final int MAX_LINKS = 5;
    }

    public interface SessionAttributes {
        // Do not remove the following session attribute entry
        // This attribute is to save the last file system selected by the user.
        // The attribute is strictly used in SamUtil to get/set user's last
        // file system selection.
        public static final String LAST_SELECTED_FS_NAME = "LastSelectedFSName";

        public static final String SAMFS_SERVER_INFO = "SAMFS_SERVER_INFO";
        public static final String FS_NAME = "FS_NAME";
        public static final
            String MOUNT_PAGE_TYPE = "SAMQFS_MOUNT_PAGE_TYPE";
        public static final String PAGE_PATH  = "PAGE_PATH";
        public static final String MEDIAFILTER_MENU = "MediaFilterMenu";
        public static final String FAULTFILTER_MENU = "FaultFilterMenu";
        public static final
            String SHARED_METADATA_SERVER = "SHARED_METADATA_SERVER";
        public static final
            String SHARED_METADATA_CLIENT = "SHARED_METADATA_CLIENT";
        public static final
            String SHARED_CLIENT_HOST = "SAMQFS_SHARED_CLIENT_HOST";
        public static final String SHARED_MEMBER = "SHARED_MEMBER";

        public static final String API_VERSION_PREFIX = "api_version_prefix";
        public static final String POLICY_NAME = "SAMQFS_POLICY_NAME";
        public static final String POLICY_TYPE = "SAMQFS_policy_type";
        public static final String CRITERIA_NUMBER  = "SAMQFS_criteria_number";
        public static final String COPY_NUMBER = "SAMQFS_copy_number";
        public static final String SHARED_MD_SERVER  = "SHARED_MD_SERVER";
        public static final String VERSION_HIGHLIGHT = "VERSION_HIGHLIGHT";
    }

    public interface PageSessionAttributes {
        public static final String PREFIX = "SAMQFS";
        public static final String SAMFS_SERVER_NAME = "SERVER_NAME";
        public static final String PAGE_NAME = "PAGE_NAME";

        public static final String LIBRARY_NAME = "SAMQFS_LIBRARY_NAME";

        // to determine if VSN Summary page is called by regular click path,
        // or directly from the tree nodes
        public static final String FROM_TREE = "SAMQFS_FROM_TREE";

        public static final String PARENT = "SAMQFS_PARENT";
        public static final String SLOT_NUMBER = "SAMQFS_SLOT_NUMBER";
        public static final String JOB_ID = "SAMQFS_JOB_ID";
        public static final String SNAPSHOT_PATH = "SAMQFS_SS_PATH";
        public static final
            String VSN_BEING_LOADED = "SAMQFS_VSN_BEING_LOADED";
        public static final String EQ = "SAMQFS_EQ";
        public static final String VSN_NAME = "SAMQFS_VSN_NAME";
        public static final String SHARE_CAPABLE = "SAMQFS_SHARE_CAPABLE";
        public static final String SEARCH_STRING = "SAMQFS_SEARCH_STRING";
        public static final String PATH_NAME = "SAMQFS_PATH_NAME";
        public static final String CONFIG_FILE_NAME = "SAMQFS_CONFIG_FILE_NAME";
        public static final
            String RESOURCE_KEY_NAME = "SAMQFS_RESOURCE_KEY_NAME";
        public static final String SHOW_CONTENT = "SAMQFS_SHOW_CONTENT";
        public static final
            String SHOW_LINE_CONTROL = "SAMQFS_SHOW_LINE_CONTROL";
        public static final String FS_NAME = "SAMQFS_FS_NAME";
        public static final String MDS_MOUNTED = "SAMQFS_MDS_MOUNTED";
        public static final String CLUSTER_NODE = "SAMQFS_CLUSTER_NODE";
        public static final String SELECTED_HOSTS = "SELECTED_HOSTS";
        public static final String SHARED_MD_SERVER = "SHARED_MD_SERVER";
        public static final String SHARED_MDS_LIST = "SHARED_MDS_LIST";

        // Do not add SAMQFS as prefix for the following attributes
        public static final String RECYCLER_NUMBER = "RECYCLER_NUMBER";
        public static final String ARCHITECTURE = "ARCHITECTURE";

        // fs tab
        public static final String FILE_SYSTEM_NAME = "SAMQFS_FILE_SYSTEM_NAME";
        public static final String FS_TYPE = "SAMQFS_FS_TYPE";
        public static final String SHARED_STATUS = "SAMQFS_SHARED_STATUS";
        public static final String ARCHIVE_TYPE = "SAMQFS_ARCHIVE_TYPE";
        public static final String SHARED_HOST_LOCAL  = "This Host";
        public static final String SHARED_HOST_CLIENT = "Shared Client";
        public static final String SHARED_HOST_NONSHARED = "NON_SHARED";
        public static final String VFSTAB_FS_TYPE = "SAMQFS_VFSTAB_FS_TYPE";
        public static final String IS_ARCHIVED = "SAMQFS_IS_ARCHIVED";
        public static final String FS_FILTER_MENU = "SAMQFS_FSFilterMenu";
        public static final String SCAN_METHOD    = "SCAN_METHOD";
        public static final String INTERVAL_VALUE = "INTERVAL_VALUE";
        public static final String INTERVAL_UNIT  = "INTERVAL_UNIT";
        public static final String LOGFILE_PATH   = "LOGFILE_PATH";

        // jobs tab
        public static final String STAGE_JOB_ID = "STAGE_JOB_ID";
        public static final String CURJOBSFILTER_MENU = "CurJobsFilterMenu";
        public static final String PENJOBSFILTER_MENU = "PenJobsFilterMenu";
        public static final String ALLJOBSFILTER_MENU = "AllJobsFilterMenu";

        // dir name used to populate nfs options
        public static final String MOUNT_POINT = "MOUNT_POINT";

        // report type
        public static final String REPORT_TYPE = "REPORT_TYPE";
        // metric type
        public static final String METRIC_TYPE = "METRIC_TYPE";
        // start date
        public static final String START_DATE = "START_DATE";
        // end date
        public static final String END_DATE = "END_DATE";
    }

    public interface MediaAttributes {
        public static final String HISTORIAN = "<Historian>";
        public static final String HISTORIAN_NAME = "Historian";
    }

    public interface Media {
        public static final int ACSLS_DEFAULT_PORT = 50004;
        public static final
            String DEFAULT_PARAM_LOCATION = "/etc/opt/SUNWsamfs/";
        public static final String STK_LIBRARY_PREFIX = "STK_LIBRARY_";
    }

    public interface ServerAttributes {
        public static final String CAPACITY_SUMMARY = "CapacitySummary";
    }

    public interface Image {
        public static final String IMAGE_DIR = "/samqfsui/images/";
        public static final
            String USAGE_BAR_DIR = IMAGE_DIR.concat("usagebar/");
        public static final
            String RED_USAGE_BAR_DIR = USAGE_BAR_DIR.concat("red/");
        public static final
            String ICON_AVAILABLE = IMAGE_DIR.concat("samqfs_available.gif");
        public static final
            String ICON_BLANK_ONE_PIXEL = IMAGE_DIR.concat("samqfs_blank.gif");
        public static final
            String ICON_BLANK = IMAGE_DIR.concat("blank.gif");
        public static final
            String ICON_NEW = IMAGE_DIR.concat("samqfs_new.gif");
        public static final
            String ICON_UPGRADE = IMAGE_DIR.concat("samqfs_upgrade.gif");
        public static final
            String ICON_REQUIRED = IMAGE_DIR.concat("required.gif");
        public static final String DIR_ICON = IMAGE_DIR.concat("folder.gif");
        public static final String FILE_ICON = IMAGE_DIR.concat("file.gif");
        public static final
            String ICON_UP_ONE_LEVEL = IMAGE_DIR.concat("up_one_level.gif");

        // File Browser Images
        public static final
            String ICON_ONLINE = IMAGE_DIR.concat("online.png");
        public static final
            String ICON_OFFLINE = IMAGE_DIR.concat("offline.png");
        public static final String ICON_PARTIAL_ONLINE =
                                   IMAGE_DIR.concat("partially_online.png");

        /**
         * use concat to construct the GIF name
         * e.g.
         * disk1_damaged.gif
         * tape2.gif
         */
        public static final
            String ICON_HONEYCOMB = IMAGE_DIR.concat("stk5800_");
        public static final
            String ICON_DISK_PREFIX =  IMAGE_DIR.concat("diskcopy_");
        public static final
            String ICON_TAPE_PREFIX =  IMAGE_DIR.concat("tapecopy_");
        public static final
            String ICON_SUFFIX = ".png";
        public static final
            String ICON_DAMAGED_SUFFIX = "_broken.png";
    }

    public interface Wizard {
        public static final String SERVER_NAME  = "SERVER_NAME";
        public static final String SERVER_VERSION = "SERVER_VERSION";
        public static final String LICENSE_TYPE = "LICENSE_TYPE";
        public static final String WIZARD_ERROR = "ERROR";
        public static final String WIZARD_ERROR_YES = "Yes";
        public static final String WIZARD_ERROR_NO = "No";
        public static final String ERROR_MESSAGE = "ERROR_MSGS";
        public static final String ERROR_CODE = "ERROR_CODE";
        public static final String ERROR_DETAIL = "ERROR_DETAIL";
        public static final String ERROR_INLINE_ALERT = "INLINE_ALERT";
        public static final String ERROR_FATAL = "FATAL_ERROR";
        public static final String ERROR_SUMMARY = "ERROR_SUMMARY";
        public static final String DETAIL_MESSAGE = "Detail_msgs";
        public static final String DETAIL_CODE  = "Detail_code";
        public static final String NO_ENTRY = "NO_ENTRY";
        public static final String SUCCESS = "SUCCESS";
        public static final String EXCEPTION = "exception";
        public static final String FINISH_RESULT  = AlertKeys.OPERATION_RESULT;
        public static final String RESULT_SUCCESS = AlertKeys.OPERATION_SUCCESS;
        public static final String RESULT_FAILED  = AlertKeys.OPERATION_FAILED;
        public static final String RESULT_FAILED_ARCFG  = "Failed_arCfg";
        public static final String RESULT_WARNING_ARCFG = "Warning_arCfg";
        public static final String FS_NAME = "fs_name";
        public static final String SLOT_NUM = "slot_num";
        public static final String FS_NAME_PARAM = "fsNameParam";
        public static final
            String DEFAULT_CATALOG_LOCATION = "/var/opt/SUNWsamfs/catalog/";

        public static final String
            MASTHEAD_SRC = SecondaryMasthead.PRODUCT_NAME_SRC;
        public static final String
            MASTHEAD_ALT = SecondaryMasthead.PRODUCT_NAME_ALT;
        public static final String
            BASENAME = ResourceProperty.BASE_NAME;
        public static final String BUNDLEID = "testBundle";
        public static final Integer WINDOW_HEIGHT = new Integer(650);
        public static final Integer WINDOW_WIDTH  = new Integer(900);

        public static final String
            ALLOCATABLE_DEVICES  = "AllAllocatableDevices";
        public static final String SELECTED_DATADEVICES = "SelectedDataDevices";
        public static final String SELECTED_METADEVICES = "SelectedMetaDevices";
        public static final String
            SELECTED_STRIPED_GROUP_DEVICES = "SelectedStripedGroupDevices";

        public static final String STRIPED_GROUP_NUM    = "stripedGroupNumber";

        public static final String WIZARD_BUTTON_KEYWORD = "SamQFSWizard";
        public static final String WIZARD_SCRIPT_KEYWORD = "SamQFSWizardScript";
        public static final int MAX_STRIPED_GROUPS = 126;
        public static final String
            AVAILABLE_STRIPED_GROUPS = "AvailableStripedGroups";

        public static final String SELECTED_MEMLIST = "SelectedMemlist";
        public static final String SELECTED_PRIMARYIP_INDEX =
            "SelectedPrimaryIPIndex";
        public static final String SELECTED_SECONDARYIP_INDEX =
            "SelectedSecondaryIPIndex";
        public static final String SELECTED_CLIENT_INDEX =
            "SelectedClientIndex";
        public static final String SELECTED_POTENTIAL_METADATA_SERVER_INDEX =
            "SelectedPotentialMetadataServerIndex";
        public static final String SELECTED_CLIENT = "SelectedClient";
        public static final String SELECTED_POTENTIAL_METADATA_SERVER_VALUE =
            "SelectedPotentialMetadataServerValue";
        public static final String SELECTED_SHARED_CLIENT =
            "SelectedSharedClient";
        public static final String SELECTED_POTENTIAL_METADATA_SERVER =
            "SelectedPotentialMetadataServer";

        public static final String
            WIZARD_RESULT_ALERT_SUMMARY = "WizardResultAlertSummary";
        public static final String
            WIZARD_RESULT_ALERT_DETAIL = "WizardResultAlertDetail";

        public static final String SERVER_API_VERSION = "SamServerAPIVersion";

        public static final int MAX_LUNS = 252;

        public static final int DEVICE_SELECTION_LIST_MAX_SIZE = 10;

        public static final String COPY_NUMBER = "copyNumber";

        public static final String DUP_POLNAME = "dupPolName";
        public static final String DUP_CRITERIA = "dupCriteria";
    }

    public interface Jobs {
        public static final String JOB_TYPE_SAMFSCK = "Jobs.jobType6";
        public static final String JOB_TYPE_METADATA_DUMP = "Jobs.jobType.dump";
    }

    public interface Filter {
        public static final String FILTER_ALL = "ALL";
    }

    public interface TableRow {
        public static final int MAX_ROW = 25;
    }

    public interface Parameters {
        public static final String FS_NAME  = "fsNameParam";
        public static final String SERVER_NAME = "serverNameParam";
        public static final String PARENT_PAGE = "parentForm";
    }

    public interface WizardParam {
        public static final String LIBRARY_NAME_PARAM = "libraryNameParam";
        public static final String SLOT_NUMBER_PARAM  = "slotNumberParam";
        public static final String EQ_VALUE_PARAM = "eqValueParam";
        public static final String SERVER_NAME_PARAM  = "serverNameParam";
        public static final String SERVER_VERSION_PARAM  = "serverVersionParam";
    }

    public interface I18N {
        public static final String I18N_OBJECT  = "i18nObj";
    }

    public interface Archive {
        public static final String DEFAULT_POLICY_NAME = "no_archive";
        public static final String NOARCHIVE_POLICY_NAME = "no_archive";
        public static final String RECYCLING_OPTION_YES = "true";
        public static final String RECYCLING_OPTION_NO = "false";

        // archiving page session attribute names
        public static final String POLICY_NAME = "SAMQFS_POLICY_NAME";
        public static final String POLICY_TYPE = "SAMQFS_policy_type";
        public static final String CRITERIA_NUMBER = "SAMQFS_criteria_number";
        public static final String COPY_NUMBER = "SAMQFS_copy_number";
        public static final String COPY_MEDIA_TYPE = "SAMQFS_copy_media_type";
        public static final String VSN_POOL_NAME = "SAMQFS_vsn_pool_name";
        public static final String POOL_MEDIA_TYPE =
            "SAMQFS_vsn_pool_media_type";
        public static final String CONTAINS_RESERVED_VSN =
            "vsnPoolContainsAReservedVSN";
    }

    public interface fs {
        public static final String MOUNT_POINT = "SAMQFS_fs_mount_point";
        public static final String FS_NAME = "SAMQFS_FILE_SYSTEM_NAME";
    }

    public interface admin {
        public static final String TASK_ID = "SAMQFS_schedule_task_id";
        public static final String TASK_NAME = "SAMQFS_schedule_task_name";
    }

    public interface Symbol {
        public static final String DAGGER = "&#8224";
        public static final String DOT = "&#8865";
    }

    public interface Cis {
        public static final String DEFAULT_CLASS_NAME = "DefaultClass";
    }

    public interface sc {
        public static final String HOST_MODEL_MAP = "host.model.map";
    }

    /**
     * Alert Type Strings for JSF:Alert component
     */
    public interface Alert {
        public static final String INFO = "information";
        public static final String SUCCESS = "success";
        public static final String WARNING = "warning";
        public static final String ERROR = "error";
    }

    public final static String UNDERBAR = "_";
}
