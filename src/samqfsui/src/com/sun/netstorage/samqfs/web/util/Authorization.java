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

// ident	$Id: Authorization.java,v 1.6 2008/03/17 14:43:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;


/**
 * JDK 1.5 enum version
 * public enum Authorization {
 *   CONFIG ("com.sun.netstorage.fsmgr.config", 31),
 *    MEDIA_OPERATOR("com.sun.netstorage.fsmgr.operator.media", 1),
 *    SAM_CONTROL("com.sun.netstorage.fsmgr.operator.sam.control", 2),
 *    FILE_OPERATOR("com.sun.netstorage.fsmgr.operator.file", 4),
 *    FILESYSTEM_OPERATOR("com.sun.netstorage.fsmgr.operator.filesystem", 8);
 *
 *    private final String _str; // string representation
 *    private final int _value; // ordinal value
 *    Authorization(String str, int val) {
 *       _str = str;
 *       _value = val;
 *    }
 *
 *    public String toString() {return _str;}
 *
 *    public int intValue() {return _value;}
 * }
 */

/* JDK 1.4 */
public final class Authorization {
    private String _name;
    private int _value;

    protected Authorization(String name, int value) {
        _name = name;
        _value = value;
    }

    public String toString() {return _name; }
    public int intValue() {return _value; }

    public static final Authorization CONFIG =
        new Authorization("com.sun.netstorage.fsmgr.config", 31);
    public static final Authorization MEDIA_OPERATOR =
        new Authorization("com.sun.netstorage.fsmgr.operator.media", 1);
    public static final Authorization SAM_CONTROL =
        new Authorization("com.sun.netstorage.fsmgr.operator.sam.control", 2);
    public static final Authorization FILE_OPERATOR =
        new Authorization("com.sun.netstorage.fsmgr.operator.file", 4);
    public static final Authorization FILESYSTEM_OPERATOR =
        new Authorization("com.sun.netstorage.fsmgr.operator.filesystem", 8);
}
