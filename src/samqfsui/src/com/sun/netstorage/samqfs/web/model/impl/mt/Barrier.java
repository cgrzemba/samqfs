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

// ident	$Id: Barrier.java,v 1.8 2008/05/16 18:39:03 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.mt;

import java.util.Vector;

/**
 * This class can be used by to synchronize ThreadPoolMember threads
 */
public class Barrier implements ThreadListener {

    // the threads that will be "watched" by the barrier
    protected Vector threads;

    // the number of threads that finished executing their tasks
    protected int nDone;

    public Barrier() {
        threads = new Vector();
        nDone = 0;
    }

    /**
     *  if the same barrier object is used for more the one sync,
     *  then the barrier needs to be reset (after each waitForAll())
     */
    public synchronized void reset() { nDone = 0; }

    public void addThread(ThreadPoolMember newThread) {
        if (threads.contains(newThread))
            return;
        newThread.setListener(this);
        threads.add(newThread);
    }

    public void addThreads(ThreadPoolMember[] newThreads) {
        for (int i = 0; i < newThreads.length; i++)
            addThread(newThreads[i]);
    }

    public synchronized void waitForAll() {
        try {
            if (nDone != threads.size()) // avoid race condition
                wait();
        } catch (InterruptedException ie) {
            System.out.println(ie); System.exit(-1);
        }
    }

    public void done(ThreadPoolMember source) {
        synchronized (this) {
            nDone++;

            if (nDone == threads.size()) {
                notify(); // wake up waitForAll()
            }
        }
    }

}
