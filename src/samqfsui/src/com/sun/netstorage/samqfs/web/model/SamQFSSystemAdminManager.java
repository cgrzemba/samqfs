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

// ident	$Id: SamQFSSystemAdminManager.java,v 1.28 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.admin.Notification;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.GenericFile;

/**
 * This interface is used to perform administrative functions on
 * an individual server.
 */
public interface SamQFSSystemAdminManager {


    /**
     * Get all the notifications.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of notifications.
     */
    public Notification[] getAllNotifications() throws SamFSException;


    /**
     * Get the notification that has the specified email address.
     * @param emailAddress
     * @throws SamFSException if anything unexpected happens.
     * @return The matching notification or <code>null</code> if
     * there is no match.
     */
    public Notification getNotification(String emailAddress)
        throws SamFSException;

    /**
     * Get a comma delimited list of the e-mail addresses that will
     * receive the given notification.
     * @param subject One of the notification subject values defined in
     * the NotifSummary class.
     * @return A comma delimited list of e-mail addresses.
     */
    public String getNotificationEmailsForSubject(int subject)
        throws SamFSException;


    /**
     * Create a new notification with the specified properties.
     * @param name
     * @param email
     * @param devDown
     * @param archIntr
     * @param reqMedia
     * @param recycle
     * @throws SamFSException if anything unexpected happens.
     * @return The new notification object.
     */
    public Notification createNotification(String name, String email,
        boolean devDown, boolean archIntr, boolean reqMedia, boolean recycle,
        boolean dumpIntr, boolean fsNospace, boolean hwmExceed,
        boolean acslsErr, boolean acslsWarn, boolean dumpWarn,
        boolean longWaitTape, boolean fsInconsistent, boolean systemHealth)
        throws SamFSException;
    /**
     * Edit the specified notification.
     * @param notification
     * @throws SamFSException if anything unexpected happens.
     */
    public void editNotification(Notification notification)
        throws SamFSException;


    /**
     * Delete the specified notification.
     * @param notification
     * @throws SamFSException if anything unexpected happens.
     */
    public void deleteNotification(Notification notification)
        throws SamFSException;


    /**
     * Since 4.4
     * Similar to the 'tail' Unix command
     * @return String []
     * @throws SamFSException if anything unexpected happens.
     */
    public String[] tailFile(String path, int maxLines) throws SamFSException;



    /**
     * Since 4.5
     * Similar to the 'cat' Unix command
     * @return String []
     * @throws SamFSException if anything unexpected happens.
     */
    public String[] getTxtFile(String path, int start, int end)
        throws SamFSException;

    public void createReport(int type, int includeSections)
        throws SamFSException;


    /**
     * Since 4.6
     * get all reports
     *
     * @return GenericFile []
     * @throws SamFSException if anything unexpected happens.
     */
    public GenericFile[] getAllReports(int reportType) throws SamFSException;

    /**
     * Since 4.6
     * get report
     * @return String (XHTML)
     * @throws SamFSException if anything unexpected happens.
     */
    public String getXHtmlReport(String reportPath, String xslFileName)
        throws SamFSException;


    // implement TaskSchedule

    /**
     * returns all the schedules of a specific type that much that provided id
     *
     */
    public Schedule [] getSpecificTasks(ScheduleTaskID id, String taskName)
        throws SamFSException;

    /**
     * add a given schedule to the configuration
     *
     * @param Schedule - the schedule to be added
     * @throws SamFSException
     */
    public void setTaskSchedule(Schedule schedule) throws SamFSException;

    /**
     * remove a named schedule from the configuration
     *
     * @param Schedule - the schedule to be removed
     * @throws SamFSException
     */
    public void removeTaskSchedule(Schedule schedule) throws SamFSException;

    /**
     * Retrieves the status of various processes and task in a SAM-FS
     * configuration.  This API is used in the Monitoring Console.
     *
     * @return String[] array of formatted strings with status information
     *
     *
     */
    public String [] getProcessStatus() throws SamFSException;

    /**
     * Retrieves the status of various components in a SAM-FS
     * configuration.  This API is used in the Monitoring Console.
     *
     * @return String[] array of formatted strings with status information
     *
     */
    public String [] getComponentStatusSummary() throws SamFSException;

    /**
     * Get the product registration information
     *
     * @return ProductRegistration
     * @throws SamFSException
     */
    public ProductRegistrationInfo getProductRegistration()
        throws SamFSException;

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
        throws SamFSException;

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
        throws SamFSException;

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
        throws SamFSException;
}
