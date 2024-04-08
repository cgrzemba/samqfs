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

// ident	$Id: TraceUtil.java,v 1.19 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.management.services.common.ConsoleConfiguration;
import com.sun.netstorage.samqfs.mgmt.Util;

/**
 * Class to use for tracing.  Tracing can be turned on/off at compile
 * time by just setting ON to true/false.
 * It can also be turned on/off at runtime by unregistering
 * TRACE_PROP_DEVICE and TRACE_PROP_LEVEL.
 */
public final class TraceUtil {

    private final static int TRACE_OFF = 0;
    private final static int TRACE_STACKSIZE = 5;

    // Private static attributes
    private static int trace_level = TRACE_OFF;
    private static int trace_stacksize = TRACE_STACKSIZE;

    // File which contains the property
    private static final String
        TRACE_PROP_LEVEL =  "com.sun.netstorage.samqfs.web.tracelevel";


    /**
     * Initialize debug tracing if enabled through system properties
     */
    public static synchronized final void initTrace() {
        // Get debug level and device; pass to trace open.
        // We set the trace file base name for the server side.

        String level = ConsoleConfiguration.getProperty(TRACE_PROP_LEVEL);

        if (level == null) {
            trace_level = TRACE_OFF;
        } else {
            try {
                trace_level = Integer.parseInt(level);
                trace_stacksize = TRACE_STACKSIZE;
            } catch (NumberFormatException numEx) {
                // bad level number
                trace_level = TRACE_OFF;
            }
        }

        Util.setNativeTraceLevel(trace_level);
        trace1(new StringBuffer(
            "Starting component debug tracing. L").append(trace_level).
            toString());
    }

    /**
     * The isOn method returns true if debug tracing is configured
     * and the debug trace level is greater than zero (tracing is
     * enabled at some level).
     *
     * @return    True if some level of tracing is enabled
     */
    public static final boolean isOn() {
        return trace_level > 0;
    }

    /**
     * Determines if tracing is outputting level 2 traces.
     *
     * @return    True if tracing is on at either level 2 or 3 false if
     * tracing is at level 1 or off.
     */
    public static final boolean isOnLevel2() {
        return trace_level > 1;
    }

    /**
     * Determines if tracing is outputting level 3 traces.
     *
     * @return    True if tracing is on at level 3, false of tracing is at
     * level 1, 2 or off.
     */
    public static final boolean isOnLevel3() {
        return trace_level > 2;
    }

    /**
     * This debug trace message method writes the message to the trace
     * log device if we are tracing at level 1.
     *
     * @param    message The debug trace message
     */
    public static final void trace1(String message) {
        if (trace_level > 0) {
            writeTraceToSyslog(message);
        }
    }

    /**
     * This debug trace message method writes the message and an exception
     * stack trace to the log if we are tracing at level 1.
     *
     * @param    message The debug trace message
     * @param    ex  The exception to trace back
     */
    public static final void trace1(String message, Throwable ex) {
        if (trace_level > 0) {
            writeTraceToSyslog(message);
            if (ex != null) {
                writeStackTraceToSyslog(ex);
            }
        }
    }

    /**
     * This debug trace message method writes the message to the trace
     * log device if we are tracing at level 2.
     *
     * @param    message The debug trace message
     */
    public static final void trace2(String message) {
        if (trace_level > 1) {
            writeTraceToSyslog(message);
        }
    }

    /**
     * This debug trace message method writes the message and an exception
     * stack trace to the log if we are tracing at level 2.
     *
     * @param    message The debug trace message
     * @param    ex  The exception to trace back
     */
    public static final void trace2(String message, Throwable ex) {
        if (trace_level > 1) {
            writeTraceToSyslog(message);
            if (ex != null) {
                writeStackTraceToSyslog(ex);
            }
        }
    }

    /**
     * This debug trace message method writes the message to the trace
     * log device if we are tracing at level 3.
     *
     * @param    message The debug trace message
     */
    public static final void trace3(String message) {
        if (trace_level > 2) {
            writeTraceToSyslog(message);
        }
    }

    /**
     * This debug trace message method writes the message and an exception
     * stack trace to the log if we are tracing at level 3.
     *
     * @param    message The debug trace message
     * @param    ex  The exception to trace back
     */
    public static final void trace3(String message, Throwable ex) {
        if (trace_level > 2) {
            writeTraceToSyslog(message);
            if (ex != null) {
                writeStackTraceToSyslog(ex);
            }
        }
    }

    public static void writeTraceToSyslog(String msg) {
        StringBuffer full_msg = new StringBuffer().append("ui:");

        full_msg.append(Thread.currentThread().getName()).append(">").
            append(getClassMethod()).append(':').append(msg);
        Util.writeToSyslog(full_msg.toString());
    }

    public static void writeStackTraceToSyslog(Throwable ex) {
        if (null != ex) {
            StackTraceElement[] elems = ex.getStackTrace();
            int j = elems.length;
            if (j > trace_stacksize) {
                j = trace_stacksize;
            }
            for (int i = 0; i < j; i++) {
                Util.writeToSyslog("    " + elems[i].toString());
            }
        }
    }

    // ********************************************************************
    //
    // Private methods
    //
    // *******************************************************************

    // Return the class name and method name that called the trace method.
    private static String getClassMethod() {
        String clmd = null;
        final int depth = 3;
        StackTraceElement[] elems = (new Exception()).getStackTrace();
        if (elems.length > depth) {
            String cn = elems[depth].getClassName();
            if (cn != null) {
                int i = cn.lastIndexOf('.');
                if (i > 0) {
                    clmd = cn.substring(i+1);
                } else {
                    clmd = cn;
                }
                String mn = elems[depth].getMethodName();
                if (mn != null) {
                    clmd = clmd + ":" + mn;
                }
            }
        }
        return (clmd);
    }

} // end TraceUtil
