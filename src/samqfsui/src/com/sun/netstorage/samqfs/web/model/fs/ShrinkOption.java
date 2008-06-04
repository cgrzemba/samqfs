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

// ident $Id: ShrinkOption.java,v 1.1 2008/06/04 18:16:10 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.fs;


import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * Options Keys:
 *  logfile = filename (default no logging)
 *  block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *  display_all_files = TRUE | FALSE (default FALSE)
 *  do_not_execute = TRUE | FALSE (default FALSE)
 *  streams = n  where 1 <= n <= 128 default 8
 */
public class ShrinkOption {

    final String KEY_LOG_FILE = "logfile";
    final String KEY_BLOCK_SIZE = "block_size";
    final String KEY_DISPLAY_ALL_FILES = "display_all_files";
    final String KEY_DO_NOT_EXECUTE = "do_not_execute";
    final String KEY_STREAMS = "streams";

    private String logFile;
    private boolean displayAllFiles, doNotExecute;
    private int blockSize, streams;

    public ShrinkOption(
        String logFile, int blockSize,
        boolean displayAllFiles, boolean doNotExecute, int streams)
        throws SamFSException {

        logFile = logFile == null ? "" : logFile;
        if (logFile.length() == 0) {
            throw new SamFSException(
                "shrink option: logFile is required.");
        }
        this.logFile = logFile;

        if (blockSize != -1 && (blockSize < 1 || blockSize > 16)) {
            throw new SamFSException(
                "shrink option: blockSize is an integer between 1 & 16!");
        }
        this.blockSize = blockSize;

        this.displayAllFiles = displayAllFiles;
        this.doNotExecute = doNotExecute;

        if (streams != -1 && (streams < 1 || streams > 128)) {
            throw new SamFSException(
                "shrink option: streams is an integer between 1 & 128!");
        }
        this.streams = streams;
    }

    /**
     * Create the shrink option string that is used in the underlying layer.
     * Used in Shrink Wizard.
     * @Override
     * @return shrink option string
     */
    public String toString() {
        StringBuffer C = new StringBuffer(",");
        StringBuffer E = new StringBuffer("=");

        StringBuffer buf = new StringBuffer();

        // LogFile (Mandatory in the GUI)
        buf.append(KEY_LOG_FILE).append(E).append(logFile);

        // Block Size
        if (blockSize != -1) {
            buf.append(C).append(KEY_BLOCK_SIZE).append(E).append(blockSize);
        }

        // Display All Files
        buf.append(C).append(KEY_DISPLAY_ALL_FILES).
            append(E).append(displayAllFiles);

        // Do Not Execute
        buf.append(C).append(KEY_DO_NOT_EXECUTE).append(E).append(doNotExecute);

        // Streams
        if (streams != -1) {
            buf.append(C).append(KEY_STREAMS).append(E).append(streams);
        }

        return buf.toString();
    }
}
