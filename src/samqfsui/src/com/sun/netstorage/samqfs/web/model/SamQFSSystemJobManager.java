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

// ident	$Id: SamQFSSystemJobManager.java,v 1.10 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
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

}
