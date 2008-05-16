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

// ident	$Id: SharedFSDetailsData.java,v 1.10 2008/05/16 18:38:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;

/**
 *  This class is the data class for FileSystemSummary actiontable
 */

public final class SharedFSDetailsData extends ArrayList {

    public String partialErrMsg = null;
    public String sharedMDServer = null;
    public String fsName = null;

    /**
     * Construct a model containing real records.
     */
    public SharedFSDetailsData(String hostName, String fsName)
        throws SamFSException, SamFSMultiHostException {
        super();
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        SamFSMultiHostException multiEx = null;

        String metaDataHostName = hostName;
        SharedMember[] sharedMember = null;
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            sharedMember =
                fsManager.getSharedMembers(metaDataHostName, fsName);
            if (sharedMember != null) {
                TraceUtil.trace3("regilar shared member length = " +
                Integer.toString(sharedMember.length));
            }

        } catch (SamFSMultiHostException e) {
            // If we have a multihost exception, we need to handle partial
            // failure.  getPartialResult() will give back some partail
            // results.
            sharedMember = (SharedMember[]) e.getPartialResult();
            if (sharedMember != null) {
                TraceUtil.trace3("exception shared member length = " +
                    Integer.toString(sharedMember.length));
            }
            SamUtil.doPrint(
                new StringBuffer("Error code is ").
                    append(e.getSAMerrno()).toString());
            multiEx = e;
            partialErrMsg = SamUtil.handleMultiHostException(e);
        }
        if (sharedMember != null && sharedMember.length > 0) {
            for (int i = 0; i < sharedMember.length; i++) {
                if (sharedMember[i].getType() == SharedMember.TYPE_MD_SERVER) {
                    sharedMDServer = sharedMember[i].getHostName();
                }

                super.add(
                    new Object[] {
                        sharedMember[i].getHostName(),
                        new Integer(sharedMember[i].getType()),
                        new Boolean(sharedMember[i].isMounted())});
            }


        } else {
            throw (multiEx);
        }
        TraceUtil.trace3("Exiting");
    }
}
