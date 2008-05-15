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

// ident	$Id: JobTypeConverter.java,v 1.2 2008/05/15 04:34:09 kilemba Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import javax.faces.component.UIComponent;
import javax.faces.context.FacesContext;
import javax.faces.convert.Converter;

public class JobTypeConverter implements Converter {

    /**
     * Thus far implementation of this method is not required for jobs. It may
     * be required for the screens that actually define the jobs.
     *
     * @param context - the current faces context.
     * @param component - the component whose value is to be converted.
     * @param value - the string representation of the current job.
     * @return Integer - the symbolic constant for the current Job type
     */
    public Object getAsObject(FacesContext context,
                              UIComponent component,
                              String value) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    public String getAsString(FacesContext context,
                              UIComponent component,
                              Object value) {
        int jobType = ((Integer)value).intValue();
        String key = null;

        switch (jobType) {
            case BaseJob.TYPE_ARCHIVE_SCAN:
                key = "job.type.archivescan";
                break;
            case BaseJob.TYPE_ARCHIVE_COPY:
                key = "job.type.archivecopy";
                break;
            case BaseJob.TYPE_ARCHIVE_FILES:
                key = "job.type.archivefiles";
                break;
            case BaseJob.TYPE_DUMP:
                key = "job.type.dump";
                break;
            case BaseJob.TYPE_ENABLE_DUMP:
                key = "job.type.enabledump";
                break;
            case BaseJob.TYPE_FSCK:
                key = "job.type.fsck";
                break;
            case BaseJob.TYPE_MOUNT:
                key = "job.type.mount";
                break;
            case BaseJob.TYPE_RECYCLE:
                key = "job.type.recycle";
                break;
            case BaseJob.TYPE_RELEASE:
                key = "job.type.release";
                break;
            case BaseJob.TYPE_RELEASE_FILES:
                key = "job.type.releasefiles";
            case BaseJob.TYPE_RESTORE:
                key = "job.type.restore";
                break;
            case BaseJob.TYPE_RESTORE_SEARCH:
                key = "job.type.restoresearch";
                break;
            case BaseJob.TYPE_RUN_EXPLORER:
                key = "job.type.runsamexplorer";
                break;
            case BaseJob.TYPE_STAGE:
                key = "job.type.stage";
                break;
            case BaseJob.TYPE_STAGE_FILES:
                key = "job.type.stagefiles";
                break;
            case BaseJob.TYPE_TPLABEL:
                key = "job.type.labeltape";
                break;
            default:
                key = "job.type.unknown";
        }

        return JSFUtil.getMessage(key);
    }
}
