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

// ident $Id: ShrinkOption.java,v 1.3 2008/11/19 22:30:43 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.fs;


import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * Options Keys:
 *  block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *  display_all_files = TRUE | FALSE (default FALSE)
 *  do_not_execute = TRUE | FALSE (default FALSE)
 *  logfile = filename (default no logging)
 *  stage_files = TRUE | FALSE (default FALSE)
 *  stage_partial = TRUE | FALSE (default FALSE)
 *  streams = n  where 1 <= n <= 128 default 8
 *
 * The integer return is a job id that will be meaningful only for
 * shared file systems.
 */
public class ShrinkOption {

    final String KEY_LOG_FILE = "logfile";
    final String KEY_BLOCK_SIZE = "block_size";
    final String KEY_DISPLAY_ALL_FILES = "display_all_files";
    final String KEY_DO_NOT_EXECUTE = "do_not_execute";
    final String KEY_STREAMS = "streams";
    final String KEY_STAGE_FILE = "stage_files";
    final String KEY_STAGE_PARTIAL = "stage_partial";

    private String logFile;
    private boolean displayAllFiles, doNotExecute;
    private boolean stageFile, stagePartial;
    private int blockSize, streams;

    public ShrinkOption(
        String logFile, int blockSize,
        boolean displayAllFiles, boolean doNotExecute, int streams,
        boolean stageFile, boolean stagePartial)
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

        this.stageFile = stageFile;
        this.stagePartial = stagePartial;
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

        // Stage File
        buf.append(C).append(KEY_STAGE_FILE).append(E).append(stageFile);

        // Partial Stage File
        buf.append(C).append(KEY_STAGE_PARTIAL).append(E).append(stagePartial);

        // Streams
        if (streams != -1) {
            buf.append(C).append(KEY_STREAMS).append(E).append(streams);
        }

        return buf.toString();
    }
}
