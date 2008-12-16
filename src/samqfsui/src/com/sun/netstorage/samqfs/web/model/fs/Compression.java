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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: Compression.java,v 1.7 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

public final class Compression {
    private int type;
    private String key;

    private Compression(int type, String key) {
        this.type = type;
        this.key = key;
    }

    // supported methods
    public int getType() { return this.type; }
    public String getKey() { return this.key; }
    public boolean equals(Object o) {
        return (o instanceof Compression &&
                this.type == ((Compression)o).getType());
    }

    public static Compression getCompression(int c) {
        switch (c) {
        case 0:
            return NONE;
        case 1:
            return GZIP;
        case 2:
            return COMPRESS;
        default:
            return NONE;
        }
    }

    // the symbolic constants
    public static final Compression NONE =
        new Compression(0, "common.compression.none");
    public static final Compression GZIP =
        new Compression(1, "common.compression.gzip");
    public static final Compression COMPRESS =
        new Compression(2, "common.compression.compress");
}
