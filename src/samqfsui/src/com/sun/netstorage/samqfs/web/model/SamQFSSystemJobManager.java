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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: SamQFSSystemJobManager.java,v 1.14 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.fs.MultiHostStatus;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;

/**
 *
 * This interface is used to manage jobs associated with an individual server.
 *
 */
public interface SamQFSSystemJobManager {

    /**
     *
     * Returns an array of jobs of the specified type.
     * The job types are defined in the BaseJob interface.
     * @param type
     * @throws SamFSException if anything unexpected happens.
     * @return An array of jobs
     *
     */
    public BaseJob[] getJobsByType(int type) throws SamFSException;

    /**
     *
     * Returns an array of jobs of the specified condition.
     * @param condition
     * @throws SamFSException if anything unexpected happens.
     * @return An array of jobs
     *
     */
    public BaseJob[] getJobsByCondition(int condition) throws SamFSException;

    /**
     *
     * Searches for a job with the specified ID.
     * @param id
     * @throws SamFSException if anything unexpected happens.
     * @return A job, or <code>null</code> if there was no match.
     *
     */
    public BaseJob getJobById(long id) throws SamFSException;

    /**
     *
     * Returns all the jobs that the system knows about
     * including current, pending and historical.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of jobs.
     *
     */
    public BaseJob[] getAllJobs() throws SamFSException;

    /**
     *
     * Cancel the specified job.
     * @param job
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void cancelJob(BaseJob job) throws SamFSException;


    /**
     * This method is used by the MultiHostStatusDisplay view to display the
     * status of jobs running in multiple shared HPC clients.
     * @since 5.0
     *
     * SAMADISPATCHJOB:-
     * Multi-Host operations will share the same set of key value pairs.
     * These key-value pairs will apply for the following operation types:
     *		UnmountClients, MountClients AddClients, RemoveClients,
     *		AddStorageNode, RemoveStorageNode, SharedGrow, SharedShrink
     *		SharedFSMountOptions
     *
     * Multi-Host Operation Keys:
     * activityid=%s (This matches the job id returned by the functions)
     * type=SAMADISPATCHJOB
     * operation=%s (sub type for each type of dispatch job)
     * starttime=%d
     * fsname=%s
     * host_count = int
     * hosts_responding = int
     * hosts_pending = int
     * status=success|partial_failure|failure|pending
     * error = overall_error_number (only present when failure or
     *			partial_failure set)
     * error_msg = overall_error_message
     * ok_hosts = hostf hoste hostc hostb hostn hostr
     * error_hosts = hosta hostd hostg
     * hosta = 31001 error message
     * hostd = 32200 error message
     * hostg = 34000 error message
     *
     * A note about multi-host error keys: The numbers used for the
     * error reporting keys map to error numbers. Error numbers are
     * returned to potentially allow localization of the messages in
     * the GUI even when the backend servers are running in the C
     * locale.
     */
    public MultiHostStatus getMultiHostStatus(long jobId)
        throws SamFSException;
}
