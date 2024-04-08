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

// ident	$Id: ThreadPool.java,v 1.10 2008/12/16 00:12:22 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.mt;

import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.Vector;

/**
 * @see    ThreadPoolMember, ThreadListener
 * This class implements a pool of threads.
 */
public class ThreadPool {

    protected final int size;
    protected ThreadPoolMember[] allThreads;
    protected Vector activeThreads;
    protected Vector availThreads;


    /** Create a pool of <i>initialSize</i> threads */
    public ThreadPool(int initialSize) {
        size = initialSize;
        allThreads = new ThreadPoolMember[size];
        activeThreads = new Vector(size);
        availThreads  = new Vector(size);
        ThreadPoolMember t;
        for (int id = 0; id < size; id++) {
            t = new ThreadPoolMember(id);
            allThreads[id] = t;
            availThreads.add(t);
        }
    }

    /** initialize (start) all threads in the pool */
    public void init() {
        TraceUtil.trace3("initializing thread pool");
        for (int i = 0; i < allThreads.length; i++) {
            ((Thread)allThreads[i]).start();
	}
        try {
            MethodDB.init(new String[] {
"com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemFSManagerImpl" });
        } catch (Exception e) {
            TraceUtil.trace3("cannot initialize MethodDB: " + e);
        }
    }

    public int getSize() { return size; }

    /** get a new thread from pool; if no thread is available, return null */
    public ThreadPoolMember getNewThread(ThreadListener tListener) {
        return this.getNewThread(tListener, null);
    }

    /** get a new thread from pool; if no thread is available, return null */
    public ThreadPoolMember getNewThread(ThreadListener tListener,
                                         String newName) {
        if (availThreads.size() == 0)
            return null;
        ThreadPoolMember t = (ThreadPoolMember) availThreads.remove(0);
        activeThreads.add(t);
        t.setListener(tListener);
        if (newName != null) t.setName(newName);
        return t;
    }


    public void releaseThreadToPool(ThreadPoolMember t) {
        t.resetException();
        t.resetResult();
        activeThreads.remove(t);
        availThreads.add(t);
    }

    public void releaseAllToPool() {
        ThreadPoolMember t;
        for (int i = activeThreads.size() - 1; i >= 0; i--) {
            t = (ThreadPoolMember) activeThreads.elementAt(i);
            releaseThreadToPool(t);
        }
    }

    public void destroy() {
        for (int i = 0; i < allThreads.length; i++)
            allThreads[i].interrupt();
    }


    /**
     * Use this section used for testing only
     */
    public static void main(String args[]) {
        // create a thread pool
        ThreadPool tp = new ThreadPool(3);

        // initialize thread pool (start threads)
        tp.init();

        // MethodDB initialization will have to move to ThreadPool.init()
        // once the client interfaces are available
        try {
        MethodDB.init(new String[] {"java.io.PrintStream"});
        } catch (ClassNotFoundException cnfe) {
            System.out.println(cnfe);
            TraceUtil.trace3(cnfe.getMessage(), cnfe);
            System.exit(-1);
        }

        // create one listener for all threads; not needed if barriers are used
        // MyThreadListener mtListener = new MyThreadListener();

        // create barrier object
        Barrier bar = new Barrier();

        // get threads from pool; specify barrier as the ThreadListener
        ThreadPoolMember tpmA, tpmB, tpmC;
        tpmA = tp.getNewThread(bar);
        tpmB = tp.getNewThread(bar, "mythread");
        tpmC = tp.getNewThread(bar);

        // add threads to barrier
        bar.addThread(tpmA); bar.addThread(tpmB); bar.addThread(tpmC);
        // OR use:  bar.addThreads(new ThreadPoolMember[] { tpmA, tpmB, tpmC});

        try {
        tpmA.startMethod(new MethodInfo("println",
                                        System.out, new Object[] {"hi.tmpA"}));
        tpmB.startMethod(new MethodInfo("println",
                                        System.out, new Object[] {"hi.tmpB"}));
        tpmC.startMethod(new MethodInfo("println",
                                        System.out, new Object[] {"hi.tmpC"}));
        } catch (NoSuchMethodException nsme) {
            System.out.println(nsme);
            TraceUtil.trace1(nsme.getMessage(), nsme);
            System.exit(-1);
        }

        TraceUtil.trace3("Waiting for all to finish Step1...");
        bar.waitForAll();
        TraceUtil.trace3("step1 done.");

        // reset the barrier so that it can be used again
        bar.reset();

        try {
        tpmA.startMethod(new MethodInfo("println", System.out,
                                        new Object[] {"hi again.tmpA"}));
        tpmB.startMethod(new MethodInfo("println", System.out,
                                        new Object[] {"hi again.tmpB"}));
        tpmC.startMethod(new MethodInfo("println", System.out,
                                        new Object[] {"hi again.tmpC"}));
        } catch (NoSuchMethodException nsme) {
            System.out.println(nsme);
            TraceUtil.trace1(nsme.getMessage(), nsme);
            System.exit(-1);
        }

        TraceUtil.trace3("Waiting for all to finish Step2...");
        bar.waitForAll();
        TraceUtil.trace3("step2 done.");

        tp.releaseAllToPool();
        TraceUtil.trace3("All threads released to pool.");

        tp.destroy();
        TraceUtil.trace3("Pool destroyed.");

    }
}
