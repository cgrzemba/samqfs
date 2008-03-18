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

// ident	$Id: ArchiveScanJobImpl.java,v 1.9 2008/03/17 14:43:49 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.arc.job.*;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJobData;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;

public class ArchiveScanJobImpl extends BaseJobImpl implements ArchiveScanJob {

    private ArFindJob jniJob = null;

    private ArchiveScanJobData regular = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData offline = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData archDone = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData copy1 = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData copy2 = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData copy3 = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData copy4 = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData dirs = new ArchiveScanJobDataImpl();

    private ArchiveScanJobData total = new ArchiveScanJobDataImpl();

    private String fsName = new String();


    public ArchiveScanJobImpl(BaseJob base,
                              String fsName,
                              ArchiveScanJobData regular,
                              ArchiveScanJobData offline,
                              ArchiveScanJobData archDone,
                              ArchiveScanJobData copy1,
                              ArchiveScanJobData copy2,
                              ArchiveScanJobData copy3,
                              ArchiveScanJobData copy4,
                              ArchiveScanJobData dirs,
                              ArchiveScanJobData total) {

        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        if (regular != null)
            this.regular = regular;

        if (offline != null)
            this.offline = offline;

        if (archDone != null)
            this.archDone = archDone;

        if (copy1 != null)
            this.copy1 = copy1;

        if (copy2 != null)
            this.copy2 = copy2;

        if (copy3 != null)
            this.copy3 = copy3;

        if (copy4 != null)
            this.copy4 = copy4;

        if (dirs != null)
            this.dirs = dirs;

        if (total != null)
            this.total = total;

        if (fsName != null)
            this.fsName = fsName;
    }

    public ArchiveScanJobImpl(ArFindJob jniJob) {
        super(jniJob);
        this.jniJob = jniJob;
        update();
    }

    public String getFileSystemName() {
        return fsName;
    }

    public ArchiveScanJobData getRegularFiles() {
        return regular;
    }

    public ArchiveScanJobData getOfflieFiles() {
        return offline;
    }

    public ArchiveScanJobData getArchDoneFiles() {
        return archDone;
    }

    public ArchiveScanJobData getCopy1() {
        return copy1;
    }

    public ArchiveScanJobData getCopy2() {
        return copy2;
    }

    public ArchiveScanJobData getCopy3() {
        return copy3;
    }

    public ArchiveScanJobData getCopy4() {
        return copy4;
    }

    public ArchiveScanJobData getDirectories() {
        return dirs;
    }

    public ArchiveScanJobData getTotal() {
        return total;
    }

    private void update() {
        if (jniJob != null) {

            Stats[] c1 = jniJob.getStats().copies;
            Stats[] c2 = jniJob.getStatsScan().copies;

            fsName = jniJob.getFSName();

            regular = new ArchiveScanJobDataImpl(jniJob.getStats().regular,
                                                 jniJob.getStatsScan().
                                                 regular);

            offline = new ArchiveScanJobDataImpl(jniJob.getStats().offline,
                                                 jniJob.getStatsScan().
                                                 offline);

            archDone = new ArchiveScanJobDataImpl(jniJob.getStats().archdone,
                                                  jniJob.getStatsScan().
                                                  archdone);

            dirs = new ArchiveScanJobDataImpl(jniJob.getStats().dirs,
                                              jniJob.getStatsScan(). dirs);

            total = new ArchiveScanJobDataImpl(jniJob.getStats().total,
                                               jniJob.getStatsScan().total);

            // TODO: I have no clue what Andrei returns; if he always returns an
            // array of 4, I'd change this weird looking code
            // TBD
            for (int i = 0; i < 4; i++) {

                if ((c1 != null) && (c1.length > i) &&
                    (c2 != null) && (c2.length > i) &&
                    (c1[i] != null) && (c2[i] != null)) {

                    switch (i) {
                    case 0:
                        copy1 = new ArchiveScanJobDataImpl(c1[i], c2[i]);
                        break;
                    case 1:
                        copy2 = new ArchiveScanJobDataImpl(c1[i], c2[i]);
                        break;
                    case 2:
                        copy3 = new ArchiveScanJobDataImpl(c1[i], c2[i]);
                        break;
                    case 3:
                        copy4 = new ArchiveScanJobDataImpl(c1[i], c2[i]);
                        break;
                    }

                }

            }

        }

    }
}
