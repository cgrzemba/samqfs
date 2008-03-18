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

// ident	$Id: LogUtil.java,v 1.10 2008/03/17 14:43:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

// Java logger classes
import java.util.logging.Logger;
import java.util.logging.Level;
import java.util.logging.FileHandler;
import java.util.logging.SimpleFormatter;

// Lockhart logger classes
import com.sun.management.services.logging.ConsoleLogService;
import com.sun.management.services.common.ConsoleConfiguration;
import java.io.IOException;

/**
 * Utility class used to write logging information to a
 * specific application defined file. Data is written in a
 * simple, human-readable format to the log file. Log file configuration
 * properties are set via smreg create --properties command.
 */
public class LogUtil {

    private static FileHandler fileHandler = null;

    /*
     * ISSUE: There is no standardized property key to use for specifying
     * a log file to write to within the Lockhart framework.
     * Applications wanting to use this class must set LOG_FILE
     * to the proper key within the configuration file.
     * TODO: See whether the base ViewBean can set this key by calling a
     * setXXX method within this class.
     */
    private static final String LOG_FILE =
        "com.sun.netstorage.samqfs.web.logfile";

    // The ConsoleLogService logger
    private static Logger logger = null;

    /*
     * Set up a handler to be used by the Logging service.
     * A 'FileHandler' is initialized here to log information to a file.
     * "samqfsuilog.file" is a configuration property that
     * contains the name of the file to write log messages to.
     */

    /*
     * Gets the logger to use and sets the handler
     */
    private static final Logger getLogger() {
        try {
            if (fileHandler == null) {
                TraceUtil.trace1("UI Logging: FILE HANDLER IS NULL");
                fileHandler = new FileHandler(
                    ConsoleConfiguration.getProperty(LOG_FILE));
                fileHandler.setFormatter(new SimpleFormatter());
            }
        } catch (IOException ioex) {
            TraceUtil.trace1("UI Logging: IOEXCEPTION CAUGHT IN LOGGER");
            // we can't log it :)
            return null;
        }

        if (logger != null) {
            return logger;
        }
        logger = ConsoleLogService.getConsoleLogger();
        logger.addHandler(fileHandler);
        return logger;
    }

    /**
     * Log an INFO message containing the class name, the method name, and
     * a message.
     */
    public static void info(Class clazz, String methodName, String message) {
        Logger logger = getLogger();
        if (logger == null) {
            return;
        }
        logger.logp(
            Level.INFO,
            clazz.getName(),
            methodName,
            message);
    }

    /**
     * Log an INFO message containing the class name, the method name, and
     * a message.
     */
    public static void info(
        Object objectClass,
        String methodName,
        String message) {
        info(objectClass.getClass(), methodName, message);
    }

    /**
     * Log an INFO message containing a message.
     */
    public static void info(String message) {
        Logger logger = getLogger();
        if (logger == null) {
            return;
        }
        logger.log(Level.INFO, message);
    }

    /**
     * Log a WARNING message containing the class name, the method name,
     * a message, and a throwable.
     */
    public static void warn(
        Class clazz,
        String method,
        String message,
        Throwable throwable) {

        Logger logger = getLogger();
        if (logger == null) {
            return;
        }
        logger.logp(
            Level.WARNING,
            clazz.getName(),
            method,
            message,
            throwable);
    }

    /**
     * Log a WARNING message containing the class name, the method name,
     * a message, and a throwable.
     */
    public static void warn(
        Object objectClass,
        String method,
        String message,
        Throwable throwable) {
        warn(objectClass.getClass(), method, message, throwable);
    }

    /**
     * Log a WARNING message containing a message.
     */
    public static void warn(String message) {
        Logger logger = getLogger();
        if (logger == null) {
            return;
        }
        logger.log(Level.WARNING, message);
    }

    /**
     * Log an ERROR message containing the class name, the method name,
     * and a message.
     */
    public static void error(
        Object objectClass,
        String method,
        String message) {
        error(objectClass.getClass(), method, message);
    }

    /**
     * Log an ERROR message containing the class name, the method name,
     * and a message.
     */
    public static void error(Class clazz, String method, String message) {
        Logger logger = getLogger();
        message = message == null ? "<empty message>" : message;
        method  = method  == null ? "<unknown method>" : method;

        if (logger == null) {
            return;
        }
        logger.logp(
            Level.SEVERE,
            (clazz == null ? "<unknown class>" : clazz.getName()),
            method,
            message);
    }

    /**
     * Log an ERROR message containing the class name, and an exception.
     */
    public static void error(Class clazz, Exception cme) {
        error(clazz, "", cme.toString());
    }

    /**
     * Log an ERROR message containing the class name, and an exception.
     */
    public static void error(Object objectClazz, Exception cme) {
        error(objectClazz.getClass(), cme);
    }

    /**
     * Log an ERROR message containing a message.
     */
    public static void error(String message) {
        Logger logger = getLogger();
        if (logger == null) {
            return;
        }
        logger.log(Level.SEVERE, message);
    }
}
