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
 * or https://illumos.org/license/CDDL.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: SamQFSSystemAdminManagerImpl.java,v 1.40 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.GetList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.NotifSummary;
import com.sun.netstorage.samqfs.mgmt.adm.Report;
import com.sun.netstorage.samqfs.mgmt.adm.SysInfo;
import com.sun.netstorage.samqfs.mgmt.adm.TaskSchedule;
import com.sun.netstorage.samqfs.mgmt.reg.Register;
import com.sun.netstorage.samqfs.web.model.ProductRegistrationInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAdminManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Configuration;
import com.sun.netstorage.samqfs.web.model.admin.FileMetricSchedule;
import com.sun.netstorage.samqfs.web.model.admin.Notification;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.GenericFile;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.model.impl.jni.admin.NotificationImpl;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.Filter;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.XmlConvertor;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Properties;
import javax.xml.transform.TransformerException;

public class SamQFSSystemAdminManagerImpl implements SamQFSSystemAdminManager {

    private SamQFSSystemModelImpl theModel;
    private HashMap notifications = new HashMap();
    private Configuration config = null;

    public SamQFSSystemAdminManagerImpl(SamQFSSystemModel model) {
        theModel = (SamQFSSystemModelImpl) model;
    }


    public Notification[] getAllNotifications() throws SamFSException {

        notifications.clear();
        NotifSummary[] n = NotifSummary.get(theModel.getJniContext());
        if ((n != null) && (n.length > 0)) {
            for (int i = 0; i < n.length; i++) {
                if (n[i].getEmailAddr().charAt(0) == '$') {
                    StringBuffer buf = new StringBuffer();
                    buf.append(n[i].getEmailAddr()).append("[").
                        append(n[i].getAdminName()).append("]");
                    n[i].setEmailAddr(buf.toString());
                    notifications.put(buf.toString(),
                       new NotificationImpl(n[i]));
                } else {
                    notifications.put(n[i].getEmailAddr(),
                       new NotificationImpl(n[i]));
                }
            }
        }

        return (Notification[]) notifications.values()
                                .toArray(new Notification[0]);
    }


    public Notification getNotification(String emailAddress)
        throws SamFSException {

        getAllNotifications();
        return (Notification) notifications.get(emailAddress);
    }

    public String getNotificationEmailsForSubject(int subject)
        throws SamFSException {

        return NotifSummary.getEmailAddrsForSubject(theModel.getJniContext(),
                                                    subject);
    }

    public Notification createNotification(String name, String emailAddress,
        boolean devDown, boolean archIntr, boolean reqMedia, boolean recycle,
        boolean dumpIntr, boolean fsFull, boolean hwmExceed,
        boolean acslsErr, boolean acslsWarn, boolean dumpWarn,
        boolean longWaitTape, boolean fsInconsistent, boolean systemHealth)
        throws SamFSException {

        boolean[] bools = new boolean[13];
        bools[NotifSummary.NOTIF_SUBJ_DEVICEDOWN] = devDown;
        bools[NotifSummary.NOTIF_SUBJ_ARCHINTR] = archIntr;
        bools[NotifSummary.NOTIF_SUBJ_MEDREQD] = reqMedia;
        bools[NotifSummary.NOTIF_SUBJ_RECYSTATRC] = recycle;
        bools[NotifSummary.NOTIF_SUBJ_DUMPINTR] = dumpIntr;
        bools[NotifSummary.NOTIF_SUBJ_FSNOSPACE] = fsFull;
        bools[NotifSummary.NOTIF_SUBJ_HWMEXCEED] = hwmExceed;
        bools[NotifSummary.NOTIF_SUBJ_ACSLSERR] = acslsErr;
        bools[NotifSummary.NOTIF_SUBJ_ACSLSWARN] = acslsWarn;
        bools[NotifSummary.NOTIF_SUBJ_DUMPWARN] = dumpWarn;
        bools[NotifSummary.NOTIF_SUBJ_LONGWAITTAPE] = longWaitTape;
        bools[NotifSummary.NOTIF_SUBJ_FSINCONSISTENT] = fsInconsistent;
        bools[NotifSummary.NOTIF_SUBJ_SYSTEMHEALTH] = systemHealth;

        NotifSummary jniNotif = new NotifSummary(name, emailAddress, bools);
        Notification n = new NotificationImpl(jniNotif);
        NotifSummary.add(theModel.getJniContext(), jniNotif);
        notifications.remove(emailAddress);
        notifications.put(emailAddress, n);

        return n;
    }

    public void editNotification(Notification notification)
        throws SamFSException {

        boolean[] bools = new boolean[13];
        bools[NotifSummary.NOTIF_SUBJ_DEVICEDOWN] =
            notification.isDeviceDownNotify();
        bools[NotifSummary.NOTIF_SUBJ_ARCHINTR] =
            notification.isArchiverInterruptNotify();
        bools[NotifSummary.NOTIF_SUBJ_MEDREQD] =
            notification.isReqMediaNotify();
        bools[NotifSummary.NOTIF_SUBJ_RECYSTATRC] =
            notification.isRecycleNotify();
        bools[NotifSummary.NOTIF_SUBJ_DUMPINTR] =
            notification.isDumpInterruptNotify();
        bools[NotifSummary.NOTIF_SUBJ_FSNOSPACE] =
            notification.isFsNospaceNotify();
        bools[NotifSummary.NOTIF_SUBJ_HWMEXCEED] =
            notification.isHwmExceedNotify();
        bools[NotifSummary.NOTIF_SUBJ_ACSLSERR] =
            notification.isAcslsErrNotify();
        bools[NotifSummary.NOTIF_SUBJ_ACSLSWARN] =
            notification.isAcslsWarnNotify();
        bools[NotifSummary.NOTIF_SUBJ_DUMPWARN] =
            notification.isDumpWarnNotify();
        bools[NotifSummary.NOTIF_SUBJ_LONGWAITTAPE] =
            notification.isLongWaitTapeNotify();
        bools[NotifSummary.NOTIF_SUBJ_FSINCONSISTENT] =
            notification.isFsInconsistentNotify();
        bools[NotifSummary.NOTIF_SUBJ_SYSTEMHEALTH] =
            notification.isSystemHealthNotify();

        NotifSummary jniNotif =
            new NotifSummary(notification.getName(),
                             notification.getEmailAddress(),
                             bools);
        NotifSummary.modify(theModel.getJniContext(),
                            ((NotificationImpl) notification).
                            getOrigEmailAddress(),
                            jniNotif);

        notifications.remove(notification.getEmailAddress());
        notifications.put(notification.getEmailAddress(),
                          new NotificationImpl(jniNotif));
    }


    public void deleteNotification(Notification notification)
        throws SamFSException {

        NotifSummary.delete(theModel.getJniContext(),
                            ((NotificationImpl) notification).
                            getJniNotif());
        notifications.remove(notification.getEmailAddress());
    }

    /**
     * Similar to the 'tail' Unix command
     */
    public String[] tailFile(String path, int maxLines) throws SamFSException {
        return FileUtil.tailFile(theModel.getJniContext(), path, maxLines);
    }


    /**
     * Since 4.5
     * Similar to the 'cat' Unix command
     */
    public String[] getTxtFile(String path, int start, int end)
        throws SamFSException {
        return FileUtil.getTxtFile(theModel.getJniContext(), path, start, end);
    }


    public void createReport(int type, int includeSections)
        throws SamFSException {
        Report.generate(theModel.getJniContext(),
            type, includeSections, null, null);
    }

    public GenericFile[] getAllReports(int reportType)
        throws SamFSException {

        String namePattern = null;
        switch (reportType) {
            case Report.TYPE_ACSLS:
                // xml filenames starting with acsls-
                namePattern = "acsls-*.xml";
                break;
            case Report.TYPE_FS:
                // xml filenames starting with fs-
                namePattern = "fs-*.xml";
                break;
            case Report.TYPE_MEDIA:
                // xml filenames starting with media-
                namePattern = "media-*.xml";
                break;
            default:
                // all xml files in Reports directory
                namePattern = "*.xml";
        }
        Filter filter = new Filter();
        filter.filterOnNamePattern(true, namePattern);

        ArrayList list = new ArrayList();
        int requiredDetails = FileUtil.FNAME | FileUtil.SIZE;

        // If the Report.REPORTS_DIR does not exist, it is not an error
        try {
            GetList dirEntries =
                FileUtil.listCollectFileDetails(
                    theModel.getJniContext(),
                    "", // no fsname
                    "", // no snappath
                    Report.REPORTS_DIR,
                    null, // no startFile (get all)
                    1024, // MAX entries
                    requiredDetails,
                    filter.toString());

            String[] reports = dirEntries.getFileDetails();

            if (reports != null) {
                for (int i = 0; i < reports.length; i++) {
                    TraceUtil.trace3("Report: " + reports[i]);
                    GenericFile report = new GenericFile(reports[i]);
                    // desc is not filled in C layer, so fill it here
                    String name = report.getName();
                    // name is of the format type-[host]-date.xml
                    int delimIndex = name.indexOf("-");
                    String type = name.substring(0, delimIndex);
                    report.setDescription(
                        SamUtil.getResourceString("reports.type.desc."+ type));

                    list.add(report);
                }
            }
        } catch (SamFSException samEx) {
            if (samEx.getSAMerrno() ==  31212) { // SE_NOT_A_DIR
                return ((GenericFile[]) list.toArray(new GenericFile[0]));
                // empty list
            }
            throw samEx;
        }

        return ((GenericFile[]) list.toArray(new GenericFile[0]));
    }

    /**
     * Since 4.6
     * get report
     * @return String (XHTML)
     * @throws SamFSException if anything unexpected happens.
     */
    public String getXHtmlReport(String reportPath, String xslFileName)
        throws SamFSException {

        String xhtmlStr = "";
        String[] strArr = tailFile(reportPath, 500);
        String xmlStr = SamUtil.array2String(strArr, "");

        try {
            InputStream is = RequestManager.getRequestContext()
                .getServletContext().getResourceAsStream(xslFileName);

            if (xmlStr != null && xmlStr.length() > 0) {
                xhtmlStr = XmlConvertor.convert2Xhtml(xmlStr, is);
            }
        } catch (IOException ioEx) {
            throw new SamFSException(ioEx.getLocalizedMessage());
        } catch (TransformerException trEx) {
            throw new SamFSException(trEx.getLocalizedMessage());
        }
        return xhtmlStr;
    }

    // implement com.sun.netstorage.samqfs.mgmt.admn.TaskSchedule

    /**
     * returns all the schedules of a specific type that much that provided id
     *
     */
    public Schedule [] getSpecificTasks(ScheduleTaskID id, String taskName)
        throws SamFSException {
        String taskId = id == null ? null : id.getId();

        String [] schedule =
            TaskSchedule.getSpecificTasks(theModel.getJniContext(),
                                          taskId,
                                          taskName);

        return createSchedules(schedule);
    }

    /**
     * add a given schedule to the configuration
     *
     * @param Schedule - the schedule to be added
     * @throws SamFSException
     */
    public void setTaskSchedule(Schedule schedule) throws SamFSException {
        String rawSchedule = schedule.toString();
        TaskSchedule.setTaskSchedule(theModel.getJniContext(), rawSchedule);
    }

    /**
     * remove a named schedule from the configuration
     *
     * @param Schedule - the schedule to be removed
     * @throws SamFSException
     */
    public void removeTaskSchedule(Schedule schedule) throws SamFSException {
        String rawSchedule = schedule.toString();
        TaskSchedule.removeTaskSchedule(theModel.getJniContext(), rawSchedule);
    }

    /**
     * construct Schedule objects give an array of raw schedule strigns
     * (comma-delimited)
     */
    protected Schedule [] createSchedules(String [] rawSchedule) {
        Schedule [] sched = new Schedule[rawSchedule.length];
        for (int i = 0; i < rawSchedule.length; i++) {
            // instantiate the right Schedule object
            Properties props = ConversionUtil.strToProps(rawSchedule[i]);
            String id = props.getProperty(Schedule.TASK_ID);
            if (id != null) {
                ScheduleTaskID taskId =
                    ScheduleTaskID.getScheduleTaskID(id.trim());
                if (taskId.equals(ScheduleTaskID.SNAPSHOT)) {
                    sched[i] = new RecoveryPointSchedule(props);
                } else if (taskId.equals(ScheduleTaskID.REPORT)) {
                    sched[i] = new FileMetricSchedule(props);
                } else {
                    // instantiate the default Schedule object with no-op
                    // decodeSchedule & encodeSchedule methods
                    sched[i] = new Schedule(props) {
                        public Properties decodeSchedule(Properties props) {
                            return props;
                        }

                        public String encodeSchedule() {
                            return "";
                        }
                    };
                }
            } // if id found
        } // end for each

        return sched;
    }

    /**
     * Retrieves the status of various processes and task in a SAM-FS
     * configuration.  This API is used in the Monitoring Console.
     *
     * @return String[] array of formatted strings with status information
     *
     *
     */
    public String [] getProcessStatus() throws SamFSException {

        return SysInfo.getProcessStatus(theModel.getJniContext());
    }

    /**
     * Retrieves the status summary of various components in a SAM-FS
     * configuration.  This API is used in the Monitoring Console.
     *
     * @return String[] array of formatted strings with status information
     *
     *
     */
    public String [] getComponentStatusSummary() throws SamFSException {

        return SysInfo.getComponentStatusSummary(theModel.getJniContext());
    }

    /**
     * Get the product registration information
     *
     * @return ProductRegistration
     * @throws SamFSException
     */
    public ProductRegistrationInfo getProductRegistration()
        throws SamFSException {

        return
            new ProductRegistrationInfo(
                Register.getRegistration(theModel.getJniContext()));
    }

    /**
     * register the product (direct connection)
     * @param sun account login
     * @param password as byte[]
     * @param name
     * @param emailAddress
     * @throws SamFSException
     */
    public void registerProduct(
        String login, byte[] password, String name, String emailAddress)
        throws SamFSException {

        ProductRegistrationInfo registration =
            new ProductRegistrationInfo(login, name, emailAddress);
        Register.register(theModel.getJniContext(),
                            registration.toString(),
                            password,
                            null /* no proxy auth password */);


    }

    /**
     * register the product (proxy connection)
     * @param sun account login
     * @param password as byte[]
     * @param name
     * @param emailAddress
     * @param proxyHost
     * @param proxyPort
     * @throws SamFSException
     */
    public void registerProduct(
        String login, byte[] password, String name, String emailAddress,
        String proxyHost, int proxyPort)
        throws SamFSException {

        ProductRegistrationInfo registration =
            new ProductRegistrationInfo(
                login, name, emailAddress, proxyHost, proxyPort);
        Register.register(theModel.getJniContext(),
                            registration.toString(),
                            password,
                            null /* no proxy auth password */);
    }

    /**
     * register the product (proxy connection and auth enabled)
     * @param sun account login
     * @param password as byte[]
     * @param name
     * @param emailAddress
     * @param proxyHost
     * @param proxyPort
     * @param authUser
     * @param authPassword as byte[]
     * @throws SamFSException
     */
    public void registerProduct(
        String login, byte[] password, String name, String emailAddress,
        String proxyHost, int proxyPort,
        String authUser, byte[] authPassword)
        throws SamFSException {

        ProductRegistrationInfo registration =
            new ProductRegistrationInfo(
                login, name, emailAddress, proxyHost, proxyPort, authUser);
        Register.register(theModel.getJniContext(),
                            registration.toString(),
                            password,
                            authPassword);
    }

    /**
     * This is method is used to support the first time configuration checklist.
     * It provides the information needed to give the user feedback on the
     * completion states of the various components on the checklist.
     *
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public Configuration getConfigurationSummary() throws SamFSException {
        String rawConfig =
            SysInfo.getConfigurationSummary(theModel.getJniContext());
        Configuration config = new Configuration(rawConfig);

        return config;
    }

    /**
     * This is method is used to support the first time configuration checklist.
     * It provides the information needed to give the user feedback on the
     * completion states of the various components on the checklist.
     *
     * @param boolean refresh - refresh the configuration from the back-end of
     * use the cached values.
     *
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public Configuration getConfigurationSummary(boolean refresh)
        throws SamFSException {
        if (this.config != null && !refresh)
            return this.config;
        else
            return getConfigurationSummary();
    }

    // sample Configuration for testing purposes
    public static String rawConfig;
    static {
        StringBuffer buf = new StringBuffer();
        buf.append("lib_count=1,")
            .append("lib_names=hp-200 hp-300,")
            .append("tape_count=25,")
            .append("qfs_count=2,")
            .append("disk_vols_count=5,")
            .append("volume_pools=2,")
            .append("object_qfs_protos=1,")
            .append("object_qfs_names=hpcfs1,")
            .append("storage_nodes=2,")
            .append("clients=4");

        rawConfig = buf.toString();
    }
}

