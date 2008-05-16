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

// ident $Id: GenericMountOptionsImpl.java,v 1.5 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.fs;

import  com.sun.netstorage.samqfs.web.model.fs.GenericMountOptions;

/**
 * common file system mount options
 */
public class GenericMountOptionsImpl implements GenericMountOptions {

    protected boolean ro, nosuid;

    public GenericMountOptionsImpl(boolean readOnly, boolean noSetUID) {
        ro = readOnly;
        nosuid = noSetUID;
    }


    public boolean isReadOnlyMount() { return ro; }
    public void setReadOnlyMount(boolean readOnly) { ro = readOnly; }

    public boolean isNoSetUID() { return nosuid; }
    public void setNoSetUID(boolean noSetUID) { nosuid = noSetUID; }

}
