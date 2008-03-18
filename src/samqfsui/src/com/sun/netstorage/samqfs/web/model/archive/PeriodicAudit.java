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

// ident	$Id: PeriodicAudit.java,v 1.3 2008/03/17 14:43:45 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

public final class PeriodicAudit {
    private String strValue;
    private String strKey;

    private PeriodicAudit(String val, String key) {
        this.strValue = val;
        this.strKey = key;
    }

    public String getStringValue() {return this.strValue; }
    public String getStringKey() {return this.strKey; }
    public boolean equals(Object o) {
        return ((PeriodicAudit)o).getStringValue().equals(this.strValue);
    }

    // audit period units
    public static final PeriodicAudit NONE =
        new PeriodicAudit("none", "archiving.dataclass.copyaudit.none");
    public static final PeriodicAudit DISK =
        new PeriodicAudit("disk_only",
                          "archiving.dataclass.copyaudit.diskonly");
    public static final PeriodicAudit ALL  =
        new PeriodicAudit("all", "archiving.dataclass.copyaudit.diskandtape");
}
