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

// ident	$Id: StkCell.java,v 1.7 2008/05/16 18:35:29 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class StkCell {

    private int minRow;
    private int maxRow;
    private int minCol;
    private int maxCol;

    /**
     * constructor
     *
     * This constructor is made public due to the simulator.  Real mode
     * presentation/logic layer code should not need to use this constructor.
     */
    public StkCell(int minRow, int maxRow, int minCol, int maxCol) {

        this.minRow = minRow;
        this.maxRow = maxRow;
        this.minCol = minCol;
        this.maxCol = maxCol;

    }

    public int getMinRow() { return minRow; }
    public int getMaxRow() { return maxRow; }
    public int getMinCol() { return minCol; }
    public int getMaxCol() { return maxCol; }

    public String toString() {
        String s = "minRow = " + minRow +
                ", maxRow = " + maxRow +
                ", minCol = " + minCol +
                ", maxCol = " + maxCol;
        return s;
    }
}
