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

// ident	$Id: TraceUtil.java,v 1.16 2008/03/17 14:43:57 am143972 Exp $

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

    private static final boolean ON = true;

    private final static int TRACE_OFF = 0;
    private final static int TRACE_STACKSIZE = 5;

    // Private static attributes
    private static boolean trace_init = false;
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
            trace_init = false;
            return;
        }

        trace_stacksize = TRACE_STACKSIZE;
        openTrace(level);
        Util.setNativeTraceLevel(trace_level);
        TraceUtil.trace1(new StringBuffer(
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
        if (TraceUtil.ON && trace_level > 0) {
            return true;
        } else {
            return false;
        }

    }

    /**
     * Determines if tracing is outputting level 2 traces.
     *
     * @return    True if tracing is on at either level 2 or 3 false if
     * tracing is at level 1 or off.
     */
    public static final boolean isOnLevel2() {
        if (TraceUtil.ON && trace_level > 1) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Determines if tracing is outputting level 3 traces.
     *
     * @return    True if tracing is on at level 3, false of tracing is at
     * level 1, 2 or off.
     */
    public static final boolean isOnLevel3() {
        if (TraceUtil.ON && trace_level > 2) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * This debug trace message method writes the message to the trace
     * log device if we are tracing at level 1.
     *
     * @param    message The debug trace message
     */
    public static final void trace1(String message) {
        if (TraceUtil.ON && trace_level > 0) {
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
        if (TraceUtil.ON && trace_level > 0) {
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
        if (TraceUtil.ON && trace_level > 1) {
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
        if (TraceUtil.ON && trace_level > 1) {
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
        if (TraceUtil.ON && trace_level > 2) {
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
        if (TraceUtil.ON && trace_level > 2) {
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

    // Internal method to open the trace log file
    /**
     * The traceOpen method initializes the client or server
     * for debug tracing.  The level of tracing is specified as an integer
     * from zero (no tracing) to three (most detailed tracing) with
     * optional characters to indicate additional message prefix informatino.
     * The trace file name argument can specify output to standard out,
     * standard error, or a specific log trace file name.  The management
     * client and management server will each specify a different trace
     * file name.  The trace file will be written to the local system's
     * /var/log directory.
     *
     * @param    level    The debug trace level: {0|1|2|3}
     * @param    filename    The debug trace log file name, stdout, or stderr
     */
    private static void openTrace(String level) {
        if (!TraceUtil.ON) {
            return;
        }

        String trace_sufx = null;
        int i;

        if (trace_init) {
            if (level != null) {
                if (Integer.parseInt(level) != trace_level) {
                    trace_level = Integer.parseInt(level);
                    return;
                }
            }
        }

        // Get the trace level and any optional flags
        trace_level = TRACE_OFF;

        // All trace/debug messages should include the method and
        // classname along with the debug message

        if (level != null) {
            Integer ix;
            try {
                ix = new Integer(level.substring(0, 1));
            } catch (Exception ex) {
                // Eat the exception
                ix = new Integer(0);
            }
            trace_level = ix.intValue();
        }

        Util.setNativeTraceLevel(trace_level);

        // If tracing turned off at runtime, just return.
        if (trace_level == 0) {
            return;
        }

        // Indicate we have initialized tracing
        trace_init = true;

    } // end openTrace


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
