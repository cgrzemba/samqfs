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

// ident	$Id: ThreadPoolMember.java,v 1.11 2008/12/16 00:12:22 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.mt;

import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.lang.reflect.InvocationTargetException;

/**
 * @see    ThreadPool, ThreadListener
 * This class implements a pool of threads.
 */
public class ThreadPoolMember extends Thread {

    protected ThreadListener listener;
    protected boolean stop, started, skipwait;
    protected int id; // unique id for this thread - assigned by the ThreadPool

    protected MethodInfo method; // the method executed by this thread
    protected Object metResult; // the result returned by the method above
    protected Throwable metException;  // the exception/error thrown (if any)

    ThreadPoolMember(int id) {
	started = false;
	// skipwait used to avoid race condition if thread has a slow start
	skipwait = false;
        this.id = id;
    }

    public int getID() { return id; }

    public Object getResult() { return metResult; }
    public Throwable getThrowable() {
        return metException;
    }
    public void setThrowable(Throwable t) {
	metException = t;
    }

    // reset methods are called internally, when thread is released to pool
    void resetException() { metException = null; }
    void resetResult() { metResult = null; }

    void setListener(ThreadListener tListener) {
        listener = tListener;
    }


    public synchronized void startMethod(MethodInfo meth) {
        method = meth;
        notify();
	if (!started)
	    skipwait = true;
    }

    public void run() {
        stop = false;

        synchronized (this) {
	    started = true;

            while (!stop) {
                try {
		    if (!skipwait) {
			wait();
                    } else {
			skipwait = false; // and stay like this forever
                    }
                } catch (InterruptedException ie) {
                    stop = true;
		    TraceUtil.trace1(this.getName() + " interrupted.");
                }

                if (!stop) {
                    try {
                        metResult = method.execute();
                    } catch (InvocationTargetException te) {
                        setThrowable(te.getCause());
                        TraceUtil.trace1(this.getName() +
                                           " target exception: " +
                                           te.getCause());
                    } catch (Exception e) {
                        setThrowable(e);
                        TraceUtil.trace1(this.getName() + " error: " + e);
                    }
                    listener.done(this);
                }

                if (isInterrupted()) {
                    stop = true;
                }
            } // while
        } // sync
    }
}
